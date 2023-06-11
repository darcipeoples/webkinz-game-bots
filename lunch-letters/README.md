# Webkinz Lunch Letters Python bot
## Demo: Level 1 versus Level 137
<img src="./play-level-1.gif" width="320" />
<img src="./play-level-137.gif" width="320" />

## Features
- Plays infinitely, restarting after each game over
- Multiple game modes:
  - Stop before hitting a score cap: best for rapdily collecting in-game money.
    - The bot gets about $2653 / hour ($480 / game, each 10.83 minutes), much higher than any other minigame I've found.
  - Stop exactly at a score cap: best for getting an unbeatable high score, since scores above 60k don't count
  - Don't stop, play forever!: best for bragging rights. It's gotten to 1.27 million points in a game and 137 levels. How far can you get?

## Instructions
1. Follow instructions in repo `README.md`
3. Configure `bot.ipynb`
   1. Modify `offset` and `game_dims` variables, if needed.
    * This program was made primarily on a Mac Book Pro using the "More Space" Display scaling (2048x1280). You may need to change the `offset` and `game_dims` variables so that it can find the game when put in the top left corner.
   2. Modify the `STOP_MODE`, `TARGET_SCORE`, and `DIFFICULTY` constants if you so desire.
4. Run all cells of the notebook
   * Press '`' (backtick) to exit the program.

## TODOs
- Could make the parsing of the letters/words faster
- Clean up old screenshots to save some storage
- Fix couldn't be broadcast together error (see test_couldnt_be_broadcast_together()) when letter is too close to edge/obscured