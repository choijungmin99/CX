# CX

A two-player vehicle arena built with Unreal Engine 5.5.4 and the Vehicle C++ template.
Collect coins, attack the other vehicle, and finish the round with the most coins.

## Run

1. Open `CX.uproject` with Unreal Engine 5.5.4.
2. Use the `CXMiningArena` map.
3. Set Play to **2 players / Play As Listen Server**, or run `PlayMultiplayer.cmd`.

Run `Build.cmd` to build the editor module. Visual Studio C++ tools and the Windows SDK are required.
On a different engine installation path, update the engine path in the command files.

## Controls

| Action | Key |
| --- | --- |
| Accelerate / brake and reverse | W / S |
| Steer | A / D |
| Handbrake | Space |
| Bumper laser | Left mouse button |
| Place mine | F |
| Recover vehicle to safe ground | Backspace |
| Chat | Enter or T |
| Cancel chat | Esc |
| Rematch after the round | R |

## Rules

- The round starts after a 10-second waiting period and lasts 120 seconds.
- Coins grant 10; speed pickups add 10 km/h to the maximum speed.
- Attacks transfer 5 coins when the target has at least 5 coins. Wheel hits temporarily reduce speed.
- Mines disable acceleration for 2 seconds. Pickups respawn after 2 seconds.
- The highest coin total wins; tied players share the win.

The arena includes ground jump ramps and an elevated road. Follow `AIR JUMP`, `2F UP`, `BYPASS`, and `DOWN` signs.

Server-side rules use GameMode, GameState, PlayerState, and ownership-checked RPCs.
The UI and chat use UMG. Gameplay comments and editor tooltips are in Korean.

Vehicle template and Starter Content assets are provided by Epic Games under the Unreal Engine license.
