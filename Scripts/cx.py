"""정책·전역 설정을 바꾸지 않고 빌드, 2인 실행, 검증, MP4 인코딩을 수행하는 도구."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
ENGINE = Path(r'C:\Program Files\Epic Games\UE_5.5')
PROJECT = ROOT / 'CX.uproject'
SAVED = ROOT / 'Saved'

def run(command, **kwargs):
    print('실행:', ' '.join(map(str, command)), flush=True)
    return subprocess.run([str(c) for c in command], cwd=ROOT, check=True, **kwargs)

def build(engine):
    run([engine / 'Engine/Build/BatchFiles/Build.bat', 'CXEditor', 'Win64', 'Development', f'-Project={PROJECT}', '-WaitMutex', '-NoHotReloadFromIDE'])

def launch(engine, port=7777, test=False, record=False):
    editor = engine / 'Engine/Binaries/Win64/UnrealEditor.exe'
    if not editor.exists():
        raise FileNotFoundError(f'UE 5.5.4 실행 파일을 찾을 수 없습니다: {editor}')
    dll = ROOT / 'Binaries/Win64/UnrealEditor-CX.dll'
    if not dll.exists():
        build(engine)
    common = [str(editor), str(PROJECT)]
    # 두 게임 창이 일반적인 Full HD 화면 안에 나란히 보이도록 배치한다.
    options = ['-game', '-windowed', '-ResX=960', '-ResY=600', '-WinY=20', '-NoSplash']
    if test:
        options += ['-CXNetTest', '-unattended']
    if record:
        options += ['-CXRecordDemo']
    logs = SAVED / 'Tests'
    logs.mkdir(parents=True, exist_ok=True)
    # 실제 조작·관찰이 필요한 게임 창이다. 녹화 역시 게임 렌더 타깃만 읽는다.
    host = subprocess.Popen(common + ['/Game/Maps/CXMiningArena?listen'] + options + [f'-port={port}', '-WinX=0', f'-abslog={logs / "Host.log"}'], cwd=ROOT)
    client = subprocess.Popen(common + [f'127.0.0.1:{port}'] + options + ['-WinX=960', f'-abslog={logs / "Client.log"}'], cwd=ROOT)
    print(f'호스트 PID={host.pid}, 게스트 PID={client.pid}', flush=True)
    return host, client

def ffmpeg_path(explicit):
    if explicit:
        return Path(explicit)
    on_path = shutil.which('ffmpeg')
    if on_path:
        return Path(on_path)
    bundled = list((ROOT / '.Tools/imageio_ffmpeg/binaries').glob('*.exe'))
    if bundled:
        return bundled[0]
    raise FileNotFoundError('재인코딩에는 FFmpeg가 필요합니다. --ffmpeg로 경로를 지정하세요. 제공된 MP4 재생에는 필요하지 않습니다.')

def encode(explicit):
    folder = SAVED / 'DemoFrames'
    output = ROOT / 'Deliverables/CX_Multiplayer_Demo.mp4'
    output.parent.mkdir(exist_ok=True)
    # 누락된 프레임이 있으면 ffmpeg가 중간에서 조용히 끝나므로 먼저 연속성을 확인한다.
    counts = []
    for side in ('HOST', 'CLIENT'):
        frames = sorted((folder / side).glob('Frame_*.png'))
        if not frames or any(p.name != f'Frame_{i:05d}.png' for i, p in enumerate(frames)):
            raise RuntimeError(f'{side}의 녹화 프레임이 없거나 중간 번호가 누락됐습니다.')
        counts.append(len(frames))
    frame_count = min(counts)
    duration = frame_count / 10.0
    temporary = output.with_name(output.stem + '.encoding.mp4')
    audio = folder / 'CLIENT/Audio.wav'
    command = [ffmpeg_path(explicit), '-y', '-framerate', '10', '-i', folder / 'HOST/Frame_%05d.png', '-framerate', '10', '-i', folder / 'CLIENT/Frame_%05d.png']
    if audio.exists():
        command += ['-i', audio]
    command += ['-filter_complex', '[0:v]scale=960:600,setsar=1[l];[1:v]scale=960:600,setsar=1[r];[l][r]hstack=inputs=2:shortest=1[v]', '-map', '[v]']
    if audio.exists():
        # 짧은 오디오 때문에 전체 시연 영상이 잘리지 않도록 부족한 구간만 무음으로 채운다.
        command += ['-map', '2:a', '-af', f'apad=whole_dur={duration:.3f}', '-c:a', 'aac', '-b:a', '160k']
    command += ['-c:v', 'libx264', '-preset', 'veryfast', '-crf', '20', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', '-frames:v', str(frame_count), '-t', f'{duration:.3f}', temporary]
    run(command)
    # 인코딩이 성공한 뒤 교체하여 실패해도 기존 제출 영상을 보존한다.
    temporary.replace(output)
    print(f'MP4 생성: {output} / {frame_count}프레임, {duration:.1f}초 ({output.stat().st_size / 1024**2:.1f} MB)')

def network_test(args):
    build(args.engine)
    report = SAVED / 'Tests/NetworkReport.json'
    report.parent.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime('%Y%m%d_%H%M%S')
    if report.exists():
        report.rename(report.with_name(f'NetworkReport_{stamp}.json'))
    frames = SAVED / 'DemoFrames'
    if args.record_demo and frames.exists():
        # 본 도구의 임시 프레임 폴더만 이동한다. 삭제하지 않으며 Saved 밖 경로는 거부한다.
        resolved = frames.resolve()
        if resolved.parent != SAVED.resolve():
            raise RuntimeError('잘못된 프레임 보관 경로입니다.')
        resolved.rename(SAVED / f'DemoFrames_{stamp}')
    processes = launch(args.engine, args.port, True, args.record_demo)
    try:
        deadline = time.monotonic() + 150
        while not report.exists():
            if time.monotonic() > deadline:
                raise TimeoutError('네트워크 검증 시간 초과. Saved/Tests 로그를 확인하세요.')
            if any(p.poll() is not None for p in processes):
                raise RuntimeError('검증 중 게임 프로세스가 종료되었습니다.')
            time.sleep(.5)
        result = json.loads(report.read_text(encoding='utf-8-sig'))
        print(json.dumps(result, ensure_ascii=False, indent=2))
        if args.record_demo:
            while not (frames / 'CLIENT/Audio.wav').exists():
                if time.monotonic() > deadline:
                    raise TimeoutError('렌더 프레임·오디오 기록 시간 초과')
                time.sleep(.5)
            encode(args.ffmpeg)
        if not result.get('passed'):
            raise RuntimeError('실패한 검증 항목이 있습니다.')
        shutil.copy2(report, ROOT / 'Docs/NetworkReport.json')
        print(f'네트워크 검증 {result["check_count"]}개 통과')
    finally:
        # 이 도구가 생성한 프로세스 핸들만 정리한다. 사용자 에디터나 다른 프로젝트는 건드리지 않는다.
        for process in reversed(processes):
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['build', 'play', 'test', 'encode', 'setup'])
    parser.add_argument('--engine', type=Path, default=ENGINE)
    parser.add_argument('--port', type=int, default=7791)
    parser.add_argument('--record-demo', action='store_true')
    parser.add_argument('--ffmpeg', default='')
    args = parser.parse_args()
    if args.command == 'build': build(args.engine)
    elif args.command == 'play': launch(args.engine, 7777)
    elif args.command == 'test': network_test(args)
    elif args.command == 'encode': encode(args.ffmpeg)
    else:
        run([args.engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe', PROJECT, f'-ExecutePythonScript={ROOT / "Scripts/setup_assignment.py"}', '-unattended', '-NoSplash'])

if __name__ == '__main__':
    main()
