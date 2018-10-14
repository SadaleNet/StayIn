# Stay in (4th Alakajam Entry of Sadale)

![GIF clip of game play](https://secret.sadale.net/blog-assets/stayIn/gameplay.gif)

[Full MP4 of the GIF above](https://secret.sadale.net/blog-assets/stayIn/gameplay.mp4)

## Links

[Youtube Video with Explanation (coming soon)](#)

[Blogpost (coming soon)](#)

## Introduction

This game was developed for a portable game console made of custom hardware. The hardware has the following specification:

* STM32F030K6T6 microcontroller (32kB flash, 4kB RAM)
* 128x64 monochrome LCD
* Buzzer and audio jack, with volume adjustment thumbwheel
* 6 buttons, including `left`, `right`, `up`, `down`, `key 1` and `key 2`
* SD card for storing game ROM and save data of the game
* 2-slot AA battery holder

Too bad! You aren't going to be able to run this game for now. Currently I'm having the only unit of this kind of game console in the world. For this reason, I'm going to describe the game in detail the following section.

## Game Mechanics

* Control: Hold `left` or `right` to walk
* Press `up` to jump. There's no double jump.
* Press `key 1` to shoot laser, which can be used to kill enemies (which will be covered later). Laser moves horizontally.
* Platforms get spawned from the bottom once a while. All platforms rise at a constant speed
* The player can stand on any platform.
* Stay within the screen or you'll lose.
	* if you get pushed to the top by the platform, you lose
	* if you jumped and reached above the top of the screen, you lose. (So don't jump if you're in somewhere high)
	* if you fall below the bottom of the screen, you lose
	* if you walk to the left or right that you get	 outside the screen, you lose
* Picking up a coin will increment the score. However, that will also randomly increase the difficulty of one of the factors below:
	* Game speed: Initially the game is slow. The game gets faster along with the difficulty
	* Platform: Initially the game does not spawn conveyors. The probability of spawning conveyors increases along with the difficulty
	* Bullets: Initially the game does not spawn bullets. The interval of spawning bullets reduces along with the difficulty
	* Mines: Initially the game does not spawn mines. The interval of spawning mines reduces along with the difficulty
	* Enemies: Initially the game does not spawn enemies. The interval of spawning enemies reduces along with the difficulty
	* (Remarks: There're 20 levels of difficulty for each of the factor above)
* Two types of platforms are available: cloud and conveyor. Conveyor will slide you to either the left or the right while you're standing on it. Each conveyor introduces varying amount of movement to the character.
* Avoid hostile entities. Touching any of them will kill the character
	* Mines spawns from the bottom. Initially it's moving slowly. After that, it speeds up to a randomized speed and moves vertically towards the top. The initial slow movement allows the player to prepare evading it.
	* Bullet spawns from the side. Initially it's moving slowly. After that, it speeds up to a randomized speed and moves horizontally across the screen.
	* Enemy spawns from the side. Horizontally, it's moving slowly and steadily towards the player. Vertically, it's always a little bit higher than the character.
		* When the character jumps, the enemy will jump along with the character. When the character falls, so will the enemy.
		* To kill it, the player has to shoot a laser towards it and jump down. As the enemy moves downward along with the character, the laser will hit it and kill it.
		* Alternatively, the character can jump up and shoot while it's at the peak. As the player falls, the enemy will fall with the character and hit the laser and get killed.
		* If the player shoots while standing on a platform, the laser will miss the enemy
		* If the player is having the same x coordinate as the enemy, the enemy will get the player and the player would die.
* Stars: There're stars on the background spawning from the bottom and slowly moving to the top. They're eyecandy and have no effect on the gameplay. It creates an illusion that the player is falling, which is the theme of the game jam.
* When you lose, your score is shown along with the previous highscore record. The highscore is saved to the SD card. Press `key 2` to restart the game.

## Why was this Game Developed?

This game was developed for a game console that I have been developing lately as an entry of the 4th Alakajam. This is a proof-of-concept game to ensure that the system library of the game console has everything required for making a functional game.

As of 14th October 2018, the second prototype of the game console is completed. The bootloader firmware is feature-complete. Minor bug may exist, tho. The next step is to design the PCB for the game console. After that, maybe I will draw a case for it. If I got the time, maybe I would also develop an emulator for it.

[Previous official blogposts about this portable game console project developed by Sadale can be found by clicking on this link](https://sadale.net/Portable%20Game%20Console/)

## License

Most part of this game is released under 2-clause BSD license. However, some external code were used by this game. All of the files inside the directory `external` may not be released under this license. Please refer to the files inside for the license(s) of the relevant files.
