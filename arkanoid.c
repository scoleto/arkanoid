#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>

#define GAME_TITLE "ARKANOID"
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_FILE "arcade_n.ttf"
#define TILE_SIZE 8
#define BRICKS_PER_LINE 11
#define NUM_BRICK_FILES 6
#define RACKET_SPEED 3
#define SPACE_WIDTH (BRICKS_PER_LINE * TILE_SIZE * 2)
#define SPACE_HEIGHT (27 * TILE_SIZE)
#define BALL_WIDTH 4
#define BALL_SPEED 2
#define MAX_ANGLE_BALL 75
#define TEX_RACKET "tex/racket.png"
#define TEX_BALL "tex/ball.png"
#define TEX_WALL_DETAIL "tex/wall_detail.png"
#define SND_RACKET_BOUND "snd/racket_bound.ogg"
#define SND_BREACK_BRICK "snd/break_brick.ogg"
#define SND_METALLIC_BRICK "snd/metallic_brick.ogg"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font;
SDL_Color white = {255, 255, 255};
SDL_Color black = {0, 0, 0};
SDL_Color colors[8] = {
    {0, 0, 255, 255},     // blue 0
    {255, 128 , 0, 255},  // orange 1
    {0, 255, 255, 255},   // cyan 2
    {255, 0, 255, 255},   // violet 3
    {255, 255, 0, 255},   // yellow 4
    {0, 255, 0, 255},     // green 5
    {255, 0, 0, 255},     // red 6
    {173, 173, 173, 255}  // grey 7
};

const int OFFSET_X = (20 - 11) * TILE_SIZE;
const int OFFSET_Y = 3 * TILE_SIZE;

const char NUM_WALL_RECTS = 3;
SDL_Rect wallRects[] = {
  {(20 - 12) * TILE_SIZE + 1, 2 * TILE_SIZE + 1, TILE_SIZE - 2, (30 - 2) * TILE_SIZE - 1},
  {(20 - 11) * TILE_SIZE - 1, 2 * TILE_SIZE + 1, 22 * TILE_SIZE + 2, TILE_SIZE - 2},
  {(20 + 11) * TILE_SIZE + 1, 2 * TILE_SIZE + 1, TILE_SIZE - 2, (30 - 2) * TILE_SIZE - 1 }};
  const char NUM_WALL_LINES = 6;
SDL_Point wallLines[] = {
  {(20 - 11) * TILE_SIZE - 6, 3 * TILE_SIZE - 6}, {(20 - 11) * TILE_SIZE - 6, 3 * TILE_SIZE + TILE_SIZE * 28},
  {(20 - 11) * TILE_SIZE - 6, 3 * TILE_SIZE - 6}, {(20 - 11) * TILE_SIZE + TILE_SIZE * 22 - 2, 3 * TILE_SIZE - 6},
  {(20 - 11) * TILE_SIZE + TILE_SIZE * 22 + 2, 3 * TILE_SIZE - 6}, {(20 - 11) * TILE_SIZE + TILE_SIZE * 22 + 2, 3 * TILE_SIZE + TILE_SIZE * 28}
  };
SDL_Texture* titleTexture = NULL;
SDL_Rect titleRect;
Mix_Music *music;
Mix_Chunk *sndEffect;

//input
char cmd_left;
char cmd_right;
char cmd_space;
char cmd_pause;
char cmd_exit;

enum GameState {ST_WAIT, ST_PLAY, ST_PAUSE, ST_GAME_OVER};
enum GameState gameState = ST_WAIT;
char done = 0;
int beginTime;
int deltaTime = 0;

struct Brick {
  char x;
  char y;
  char hits;
  char color;
};

const int NUM_BRICKS = BRICKS_PER_LINE * NUM_BRICK_FILES;
struct Brick bricks[BRICKS_PER_LINE * NUM_BRICK_FILES];

SDL_Rect racket;
SDL_Rect ball;
float ballSpeedX;
float ballSpeedY;
float ballPosX;
float ballPosY;
char ballStopped;

SDL_Texture* texRacket;
SDL_Texture* texBall;
SDL_Texture* texWallDetail;

Mix_Music *music = NULL;
Mix_Chunk *sndRacketBound = NULL;
Mix_Chunk *sndBreakBrick = NULL;
Mix_Chunk *sndMetallicBrick = NULL;
Mix_Music *musGameOver = NULL;

void resetGame();

SDL_Texture* loadTexture(char* path) {
  SDL_Texture* newTexture = NULL;
  SDL_Surface* loadedSurface = IMG_Load(path);
  if (loadedSurface == NULL) {
    printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
  } else {
    newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    if (newTexture == NULL) {
      printf("Unable to create texture %s! SDL Error: %s\n", path, SDL_GetError());
    }
    SDL_FreeSurface(loadedSurface);
  }
  return newTexture;
}

int init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		return 1;
	}
	window = SDL_CreateWindow( GAME_TITLE, SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, 640, 480,
                            SDL_WINDOW_SHOWN);// | SDL_WINDOW_FULLSCREEN);
  if (window == NULL) {
		printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		return 1;
  }

  if (TTF_Init() == -1) {
    printf("TTF_Init: %s\n", TTF_GetError());
    return 1;
  }

  renderer = SDL_CreateRenderer(window, -1,
                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
  if (renderer == NULL)
  {
      printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
      return 1;
  }
  SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

  if (!Mix_Init(MIX_INIT_MODPLUG)) {
    printf( "SDL_mixer could not be created! SDL Error: %s\n", Mix_GetError());
    return 1;
  }

  font = TTF_OpenFont(FONT_FILE, 8);
  SDL_Surface* textSurface = TTF_RenderText_Solid(font, GAME_TITLE, white);
  titleTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
  titleRect.h = textSurface->h;
  titleRect.w = textSurface->w;
  titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;
  titleRect.y = (SCREEN_HEIGHT - titleRect.h) / 2;
  SDL_FreeSurface(textSurface);

  texRacket = loadTexture(TEX_RACKET);
  texBall = loadTexture(TEX_BALL);
  texWallDetail = loadTexture(TEX_WALL_DETAIL);

  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024)==-1) {
    printf("Mix_OpenAudio: %s\n", Mix_GetError());
  }
  sndRacketBound = Mix_LoadWAV(SND_RACKET_BOUND);
  if (!sndRacketBound) {
    printf("Mix_LoadWAV: %s\n", Mix_GetError());
  }
  sndBreakBrick = Mix_LoadWAV(SND_BREACK_BRICK);
  if (!sndBreakBrick) {
    printf("Mix_LoadWAV: %s\n", Mix_GetError());
  }
  sndMetallicBrick = Mix_LoadWAV(SND_METALLIC_BRICK);
  if (!sndMetallicBrick) {
    printf("Mix_LoadWAV: %s\n", Mix_GetError());
  }

  resetGame();

  return 0;
}

void resetGame() {
  cmd_left = 0;
  cmd_right = 0;
  for (int i = 0; i < BRICKS_PER_LINE; i++) {
    bricks[i].x = i * 2;
    bricks[i].y = 4;
    bricks[i].hits = 2;
    bricks[i].color = 7;
  }
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < BRICKS_PER_LINE; j++) {
      int index = (i + 1) * BRICKS_PER_LINE + j;
      bricks[index].x = j * 2;
      bricks[index].y = i + 5;
      bricks[index].hits = 1;
      bricks[index ].color = i;
    }
  }
  racket.x = (13 - 2) * TILE_SIZE;
  racket.y = (30 - 3 - 3) * TILE_SIZE;
  racket.w = 4 * TILE_SIZE;
  racket.h = TILE_SIZE;
  ballSpeedX = 0.0f;
  ballSpeedY = 0.0f;
  ball.w = BALL_WIDTH;
  ball.h = BALL_WIDTH;
  ballStopped = 1;
}

void quit() {
  SDL_DestroyTexture(titleTexture);
  SDL_DestroyTexture(texRacket);
  SDL_DestroyTexture(texBall);
  SDL_DestroyTexture(texWallDetail);
  TTF_CloseFont(font);
  Mix_FreeMusic(music);
  Mix_FreeChunk(sndRacketBound);
  Mix_FreeChunk(sndBreakBrick);
  Mix_FreeChunk(sndMetallicBrick);
  Mix_CloseAudio();

  TTF_Quit();
  Mix_Quit();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void input() {
  cmd_space = 0;
  cmd_pause = 0;
  cmd_exit = 0;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_LEFT) {
        cmd_left = 1;
      } else if (event.key.keysym.sym == SDLK_RIGHT) {
        cmd_right = 1;
      } else if (event.key.keysym.sym == SDLK_p) {
        cmd_pause = 1;
      } else if (event.key.keysym.sym == SDLK_SPACE) {
        cmd_space = 1;
      } else if (event.key.keysym.sym == SDLK_ESCAPE) {
        done = 1;
      }
    } else if (event.type == SDL_KEYUP) {
      if (event.key.keysym.sym == SDLK_LEFT) {
        cmd_left = 0;
      } else if (event.key.keysym.sym == SDLK_RIGHT) {
        cmd_right = 0;
      }
    }
  }
}

void moveBall() {
  //walls collision
  ballPosX += ballSpeedX;
  if (ballPosX >= SPACE_WIDTH - BALL_WIDTH) {
    ballPosX = SPACE_WIDTH - BALL_WIDTH - (ballPosX - (SPACE_WIDTH - BALL_WIDTH));
    ballSpeedX *= -1;
  } else if (ballPosX < 0) {
    ballPosX = -ballPosX;
    ballSpeedX *= -1;
  }

  ballPosY += ballSpeedY;
  if (ballPosY < 0) {
    ballPosY = -ballPosY;
    ballSpeedY *= -1;
  } else if (ballPosY > SPACE_HEIGHT) {
    ballPosY = SPACE_HEIGHT * 0.05f;
    ballPosX = SPACE_WIDTH * 0.5f;
  }
  ball.x = ballPosX;
  ball.y = ballPosY;

  //racket collision
  SDL_Rect r;
  if (SDL_IntersectRect(&ball, &racket, &r)) {
    float angleBall = ((float)(ball.x - (racket.x - ball.w + 1)) / (ball.w + racket.w - 1))
            * (2.0f * MAX_ANGLE_BALL) - MAX_ANGLE_BALL ;
    float radAngleBall = angleBall * 3.1416 / 180;
    float speedModule = sqrt(ballSpeedX * ballSpeedX + ballSpeedY * ballSpeedY);
    if (ballSpeedX > 0 && ballSpeedY > 0) {
      float m = ballSpeedY / ballSpeedX;
      float y = (ball.y + ball.h) - racket.y;
      float x = ball.x + ball.w - racket.x;
      float a = y - m * x;
      if (a < 0) {
        ballPosY = racket.y - BALL_WIDTH - (ballPosY - (racket.y - BALL_WIDTH));
      } else {
        ballPosX = racket.x - ball.w;
      }
    } else if (ballSpeedX < 0 && ballSpeedY > 0) {
      float m = ballSpeedY / -ballSpeedX;
      float y = (ball.y + ball.h) - racket.y;
      float x = racket.x + racket.w - ball.x;
      float a = y - m * x;
      if (a < 0) {
        ballPosY = racket.y - BALL_WIDTH - (ballPosY - (racket.y - BALL_WIDTH));
      } else {
        ballPosX = racket.x + racket.w;
      }
    }
    ballSpeedX = sin(radAngleBall) * speedModule;
    ballSpeedY = - cos(radAngleBall) * speedModule;
    ball.x = ballPosX;
    ball.y = ballPosY;
    Mix_PlayChannel(-1, sndRacketBound, 0);
  }

  //check bricks
  SDL_Rect recBrick;
  recBrick.w = 2 * TILE_SIZE;
  recBrick.h = TILE_SIZE;
  if (ballSpeedX > 0 && ballSpeedY < 0) {
    for (int i = NUM_BRICK_FILES - 1; i >= 0; i--) {
      for (int j = 0; j < BRICKS_PER_LINE; j++) {
        int index = i * BRICKS_PER_LINE + j;
        if (bricks[index].hits > 0) {
          recBrick.x = bricks[index].x * TILE_SIZE;
          recBrick.y = bricks[index].y * TILE_SIZE;
          if (SDL_IntersectRect(&ball, &recBrick, &r)) {
            float m = (-ballSpeedY) / ballSpeedX;
            float y = recBrick.y + recBrick.h - 1 - ball.y;
            float x = (ball.x + ball.w) - recBrick.x;
            float a = y - m * x;
            if (a < 0) {
              ballSpeedY *= -1;
              /*ballPosY = (recBrick.y + recBrick.h) +
                  ((recBrick.y + recBrick.h) - (ball.y + ball.h));*/
              ballPosY = recBrick.y + recBrick.h;
              ball.y = ballPosY;
            } else {
              ballSpeedX *= -1;
              //ballPosX = recBrick.x - ((ball.x + ball.w) - recBrick.x);
              ballPosX = recBrick.x - ball.w;
              ball.x = ballPosX;
            }
            bricks[index].hits--;
            if (bricks[index].hits == 0) {
              Mix_PlayChannel(-1, sndBreakBrick, 0);
            } else {
              Mix_PlayChannel(-1, sndMetallicBrick, 0);
            }
            return;
          }
        }
      }
    }
  } else if (ballSpeedX < 0 && ballSpeedY < 0) {
    for (int i = NUM_BRICK_FILES - 1; i >= 0; i--) {
      for (int j = BRICKS_PER_LINE - 1; j >= 0; j--) {
        int index = i * BRICKS_PER_LINE + j;
        if (bricks[index].hits > 0) {
          recBrick.x = bricks[index].x * TILE_SIZE;
          recBrick.y = bricks[index].y * TILE_SIZE;
          if (SDL_IntersectRect(&ball, &recBrick, &r)) {
            float m = ballSpeedY / ballSpeedX;
            float y = recBrick.y + recBrick.h - ball.h - ball.y;
            float x = recBrick.x + recBrick.w - ball.x;
            float a = y - m * x;
            if (a < 0) {
              ballSpeedY *= -1;
              /*ballPosY = (recBrick.y + recBrick.h) +
                  ((recBrick.y + recBrick.h) - (ball.y + ball.h));*/
              ballPosY = recBrick.y + recBrick.h;
              ball.y = ballPosY;
            } else {
              ballSpeedX *= -1;
              /*ballPosX = (recBrick.x + recBrick.w) +
                  ((recBrick.x + recBrick.w) - ball.x);*/
              ballPosX = recBrick.x + recBrick.w;
              ball.x = ballPosX;
            }
            bricks[index].hits--;
            if (bricks[index].hits == 0) {
              Mix_PlayChannel(-1, sndBreakBrick, 0);
            } else {
              Mix_PlayChannel(-1, sndMetallicBrick, 0);
            }
            return;
          }
        }
      }
    }
  } else if (ballSpeedX > 0 && ballSpeedY > 0) {
    for (int i = 0; i < NUM_BRICK_FILES; i++) {
      for (int j = 0; j < BRICKS_PER_LINE; j++) {
        int index = i * BRICKS_PER_LINE + j;
        if (bricks[index].hits > 0) {
          recBrick.x = bricks[index].x * TILE_SIZE;
          recBrick.y = bricks[index].y * TILE_SIZE;
          if (SDL_IntersectRect(&ball, &recBrick, &r)) {
            float m = ballSpeedY / ballSpeedX;
            float y = (ball.y + ball.h) - recBrick.y;
            float x = (ball.x + ball.w) - recBrick.x;
            float a = y - m * x;
            if (a < 0) {
              ballSpeedY *= -1;
              ballPosY = recBrick.y - ball.h;
              ball.y = ballPosY;
            } else {
              ballSpeedX *= -1;
              ballPosX = recBrick.x - ball.w;
              ball.x = ballPosX;
            }
            bricks[index].hits--;
            if (bricks[index].hits == 0) {
              Mix_PlayChannel(-1, sndBreakBrick, 0);
            } else {
              Mix_PlayChannel(-1, sndMetallicBrick, 0);
            }
            return;
          }
        }
      }
    }
  } else if (ballSpeedX < 0 && ballSpeedY > 0) {
    for (int i = 0; i < NUM_BRICK_FILES; i++) {
      for (int j = BRICKS_PER_LINE - 1; j >= 0 ; j--) {
        int index = i * BRICKS_PER_LINE + j;
        if (bricks[index].hits > 0) {
          recBrick.x = bricks[index].x * TILE_SIZE;
          recBrick.y = bricks[index].y * TILE_SIZE;
          if (SDL_IntersectRect(&ball, &recBrick, &r)) {
            float m = ballSpeedY / -ballSpeedX;
            float y = (ball.y + ball.h) - recBrick.y;
            float x = recBrick.x + recBrick.w - ball.x;
            float a = y - m * x;
            if (a < 0) {
              ballSpeedY *= -1;
              ballPosY = recBrick.y - ball.h;
              ball.y = ballPosY;
            } else {
              ballSpeedX *= -1;
              ballPosX = recBrick.x + recBrick.w;
              ball.x = ballPosX;
            }
            bricks[index].hits--;
            if (bricks[index].hits == 0) {
              Mix_PlayChannel(-1, sndBreakBrick, 0);
            } else {
              Mix_PlayChannel(-1, sndMetallicBrick, 0);
            }
            return;
          }
        }
      }
    }
  }
}

void logicPlay() {
  if (cmd_left) {
    racket.x -= RACKET_SPEED;
    if (racket.x < 0) {
      racket.x = 0;
    }
  }

  if (cmd_right) {
    racket.x += RACKET_SPEED;
    if (racket.x >= SPACE_WIDTH - racket.w) {
      racket.x = SPACE_WIDTH - racket.w;
    }
  }

  if (cmd_space && ballStopped) {
    ballSpeedX = BALL_SPEED;
    ballSpeedY = -BALL_SPEED;
    ballStopped = 0;
  }

  if (ballStopped) {
    ballPosX = racket.x + (racket.w - BALL_WIDTH) / 2;
    ballPosY = racket.y - BALL_WIDTH;
    ball.x = ballPosX;
    ball.y = ballPosY;
  } else {
    moveBall();
  }
}

void logic() {
  switch (gameState) {
  case ST_WAIT:
    if (cmd_space) {
      resetGame();
      gameState = ST_PLAY;
    }
    break;
  case ST_PLAY:
    logicPlay();
    break;
  case ST_GAME_OVER:
    break;
  }
}

void render() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  switch (gameState) {
  case ST_WAIT:
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    //break;

  case ST_PLAY:
    SDL_SetRenderDrawColor(renderer, 176, 176, 176, 0);

    // walls
    SDL_RenderFillRects(renderer, wallRects, NUM_WALL_RECTS);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
    SDL_RenderDrawLines(renderer, wallLines, NUM_WALL_LINES);
    SDL_Rect r_detail;
    r_detail.w = TILE_SIZE;
    r_detail.h = TILE_SIZE * 3 + 1;
    //left wall
    r_detail.x = OFFSET_X - TILE_SIZE;
    r_detail.y = OFFSET_Y + TILE_SIZE * 2;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    r_detail.y = OFFSET_Y + TILE_SIZE * 9;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    r_detail.y = OFFSET_Y + TILE_SIZE * 16;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    r_detail.y = OFFSET_Y + TILE_SIZE * 23;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    //right wall
    r_detail.x = OFFSET_X + TILE_SIZE * 22;
    r_detail.y = OFFSET_Y + TILE_SIZE * 2;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    r_detail.y = OFFSET_Y + TILE_SIZE * 9;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    r_detail.y = OFFSET_Y + TILE_SIZE * 16;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    r_detail.y = OFFSET_Y + TILE_SIZE * 23;
    SDL_RenderCopy(renderer, texWallDetail, NULL, &r_detail);
    //top wall
    r_detail.y = OFFSET_Y;
    SDL_Point rotPoint = {0, 0};
    r_detail.x = OFFSET_X + TILE_SIZE * 4;
    SDL_RenderCopyEx(renderer, texWallDetail, NULL, &r_detail,
        -90, &rotPoint, SDL_FLIP_HORIZONTAL);
    r_detail.x = OFFSET_X + TILE_SIZE * 15;
    SDL_RenderCopyEx(renderer, texWallDetail, NULL, &r_detail,
        -90, &rotPoint, SDL_FLIP_HORIZONTAL);

    // bricks
    for (int i = 0; i < NUM_BRICKS; i++) {
      if (bricks[i].hits > 0) {
        SDL_Rect r;
        r.x = bricks[i].x * TILE_SIZE + OFFSET_X;
        r.y = bricks[i].y * TILE_SIZE + OFFSET_Y;
        r.w = TILE_SIZE * 2;
        r.h = TILE_SIZE;
        SDL_SetRenderDrawColor(
            renderer,
            colors[bricks[i].color].r,
            colors[bricks[i].color].g,
            colors[bricks[i].color].b,
            colors[bricks[i].color].a);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer,
            black.r,
            black.g,
            black.b,
            black.a);
        SDL_RenderDrawLine(renderer, r.x, r.y + r.h - 1,
            r.x + r.w - 1, r.y + r.h - 1);
        SDL_RenderDrawLine(renderer, r.x + r.w - 1, r.y,
            r.x + r.w - 1, r.y + r.h - 1 );
      }
    }

    // racket
    SDL_Rect r;
    r.x = racket.x + OFFSET_X;
    r.y = racket.y + OFFSET_Y;
    r.w = racket.w;
    r.h = racket.h;
    /*SDL_SetRenderDrawColor(
        renderer,
        colors[6].r,
        colors[6].g,
        colors[6].b,
        colors[6].a);
    SDL_RenderFillRect(renderer, &r);*/
    SDL_RenderCopy(renderer, texRacket, NULL, &r);

    // ball
    r.x = ball.x + OFFSET_X;
    r.y = ball.y + OFFSET_Y;
    r.w = ball.w;
    r.h = ball.h;
    /*SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &r);*/
    SDL_RenderCopy(renderer, texBall, NULL, &r);

  case ST_GAME_OVER:
    break;
  }

  SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
  if (init())
    return 1;

	while (!done) {
    beginTime = SDL_GetTicks();
    input();
    logic();
    render();
    deltaTime = SDL_GetTicks() - beginTime;
	}

  quit();

  return 0;
}

