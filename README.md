# Trail Clash
Game trailer: https://youtu.be/SlYqqkHDUAg

# About
TrailClash - a game in the client-server model and has been conducted at KTH Flemingsberg by six students within the course HI1038 - Project Course in Data and Network Engineering during the spring semester of 2023. The inspiration for the game TrailClash is based on the longing for classic computer games that are easy to play and understand. The result is a top-down arcade game for a total of four players.

Each match in the game consists of three rounds, and each player is assigned points based on their placement in each round. The player's main task is to survive as long as possible by avoiding collisions with other players and the walls. The player who has collected the most points over the three rounds wins the match. 

The produced prototype meets all of KTH's specifications and achieves the project team's overall internal goals regarding game development and project implementation. However, some of the project team's internal goals were adjusted and modified due to time constraints.

The game is created in C.

# Get started
### Requriments
- You have to have a functional terminal; powershell, msys2 mingw64, git bash.
- Installed C, [Download C](https://github.com/Makerspace-KTH/c_programing_intro)
- Installed SDL, [Download SDL](https://github.com/Makerspace-KTH/sdl_hello)
- VS Code

### How to use
Open your terminal.

Run the makefile based on your OS to build the trailClash executable

Windows:

    $ mingw32-make.exe

Linux / macOS:

    $ make

Executable file has been created in the same folder.

Execute the game as player:

    $ ./TrailClash.exe

Execute the game as the server:

    $ ./server.exe


# Highlights
- SDL2 development library
- UDP-protocal

# Credits

[Tarik Bratic](https://github.com/tarik-bratic), [Filip Kliemert](https://github.com/Filbon), [Linus Blomberg](https://github.com/TSPLB), [Tobias Erlandsson](https://github.com/tobbe00), [Roman Luis Furman](https://github.com/RomFur), [Adil Chohan](https://github.com/aac1999)
