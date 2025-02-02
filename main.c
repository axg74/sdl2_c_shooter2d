#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

const int GAME_WIDTH = 368;
const int GAME_HEIGHT = 240;
const int GAME_SCALE = 3;

// ############################################################################

bool init_game(int width, int height, int scale);
void quit_game(void);
bool game_preload(void);
void game_unload(void);
void game_update(void);
void game_draw(void);
void game_loop(void);
void reset_input(void);
void check_events(void);
void check_keyboard(int keycode, bool flag);
bool init_screen(int width, int height, int scale);

bool keydown_back(void);
bool keydown_up(void);
bool keydown_down(void);
bool keydown_left(void);
bool keydown_right(void);
bool keydown_fire1(void);
bool keydown_fire2(void);

void cls(int red, int green, int blue);
void flip(void);
void draw_backbuffer(void);
SDL_Texture* load_bmp(char *filename);
void unload_bmp(SDL_Texture *texture);
void draw_subimage_rect(SDL_Texture *texture, int x, int y, int width, int height, int source_x, int source_y);

void delta_time_init(void);
void delta_time_update(void);

void player_init(void);

// ############################################################################

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *backbuffer;

    int width;
    int height;
    int offset_x;
    int offset_y;
    int scale;
    bool is_full_screen;

} Screen;

// ############################################################################

typedef struct
{
    bool back;
    bool start;
    bool left;
    bool right;
    bool up;
    bool down;
    bool fire1;
    bool fire2;
} Input;

// ############################################################################
// sprite structures and data
typedef struct
{
    bool active;
    float x;
    float y;
    int width;
    int height;
    int source_x;
    int source_y;
    float speed_x;
    float speed_y;
    int dir_x;
    int dir_y;
    int type;
    int user_value1_float;
    SDL_Texture *texture;
} Sprite;

SDL_Texture *spritesheet;

void sprite_init(Sprite *spr, float x, float y, int width, int height, SDL_Texture *texture, int source_x, int source_y);
void draw_sprite(Sprite *spr);
void sprite_set_speed(Sprite *spr, float x, float y);

// ############################################################################
// tilemap structures and data
typedef struct
{
    // current position in pixel
    float x;
    float y;

    // size in tiles
    int width;
    int height;

    SDL_Texture *texture;

    int tilesize;
    int tilesheet_width;
    int tilesheet_height;
    int tile_postab[1024 * 2];
} Tilemap;

SDL_Texture *tilesheet;
Tilemap tilemap;

void tilemap_init(int width, int height, SDL_Texture *texture);
bool tilemap_load(const char *filename);
void tilemap_draw(void);

// ############################################################################
// delta timing
float delta_time;
float delta_time_last;

// ############################################################################
// game components
Screen game_screen;
Input game_input;

// ############################################################################
// game sprite objects
Sprite player_sprite;

// ############################################################################

int main(int argc, char *argv[])
{
    if(!init_game(GAME_WIDTH, GAME_HEIGHT, GAME_SCALE))
    {
        quit_game();
        return -1;
    }

    if(!game_preload())
    {
        quit_game();
        return -2;
    }

    game_loop();

    quit_game();
    return 0;
}

// ############################################################################
// PLAYER FUNCTIONS
// ############################################################################
void player_init()
{
    float speed = 60.0f;

    sprite_init(&player_sprite, 100.0f, 100.0f, 24, 16, spritesheet, 48, 16);
    sprite_set_speed(&player_sprite, speed, speed);
}

void player_draw()
{
    draw_sprite(&player_sprite);
}

void player_update()
{
    float dx = 0, dy = 0;

    if (keydown_right()) dx = 1;
    if (keydown_left()) dx = -1;
    if (keydown_up()) dy = -1;
    if (keydown_down()) dy = 1;

    player_sprite.x += dx * player_sprite.speed_x * delta_time;
    player_sprite.y += dy * player_sprite.speed_y * delta_time;

    if (player_sprite.x < 0) player_sprite.x = 0;
    if (player_sprite.y < 0) player_sprite.y = 0;
    if (player_sprite.x + player_sprite.width > GAME_WIDTH) player_sprite.x = GAME_WIDTH - player_sprite.width;
    if (player_sprite.y + player_sprite.height > GAME_HEIGHT) player_sprite.y = GAME_HEIGHT - player_sprite.height;
}

// ############################################################################
// SPECIFIC GRAPHICS GAME FUNCTIONS
// ############################################################################
bool game_preload()
{
    spritesheet = load_bmp("data/spritesheet1.bmp");
    if (spritesheet == NULL) return false;

    tilesheet = load_bmp("data/tilesheet1.bmp");
    if (tilesheet == NULL) return false;

    tilemap_load("level1.tmx");

    return true;
}

// CLEAN UP ALL LOADED DATA
void game_unload()
{
    unload_bmp(spritesheet);
    unload_bmp(tilesheet);
}

// DRAWING THE GAME
void game_draw()
{
    cls(0, 0, 100);

    tilemap_draw();
    player_draw();
    draw_backbuffer();
    flip();
}

// UPDATING THE GAME
void game_update()
{
    // scrolling
    if (tilemap.x - (tilemap.width * tilemap.tilesize) < (tilemap.width * tilemap.tilesize))
    {
        tilemap.x += 15 * delta_time;
    }

    player_update();
}

// GAME-LOOP
void game_loop()
{
    bool is_game_running;

    player_init();
    tilemap_init(40, 14, tilesheet);

    is_game_running = true;

    while(is_game_running)
    {
        delta_time_update();
        check_events();

        game_update();
        game_draw();

        if (keydown_back())
        {
            is_game_running = false;
        }
    }
}

// INIT FUNCTIONS
bool init_game(int width, int height, int scale)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if(!init_screen(width, height, scale)) return false;

    reset_input();
    delta_time_init();
    return true;
}

bool init_screen(int width, int height, int scale)
{
    game_screen.is_full_screen = false;
    game_screen.width = width;
    game_screen.height = height;
    game_screen.scale = scale;

    game_screen.window = SDL_CreateWindow("game",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          width * scale, height * scale,
                                          SDL_WINDOW_SHOWN);
    if (game_screen.window == NULL) return false;

    game_screen.renderer = SDL_CreateRenderer(game_screen.window, -1, SDL_RENDERER_ACCELERATED  | SDL_RENDERER_PRESENTVSYNC);
    if (game_screen.renderer == NULL) return false;

    game_screen.backbuffer = SDL_CreateTexture(game_screen.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    if (game_screen.backbuffer == NULL) return false;

    return true;
}

// QUIT FUNCTIONS
void quit_game()
{
    if (game_screen.backbuffer != NULL) SDL_DestroyTexture(game_screen.backbuffer);
    if (game_screen.renderer != NULL) SDL_DestroyRenderer(game_screen.renderer);
    if (game_screen.window != NULL) SDL_DestroyWindow(game_screen.window);

    SDL_Quit();
}

// ############################################################################
// CORE 2D GRAPHICS GAME FUNCTIONS
// ############################################################################
void cls(int red, int green, int blue)
{
    SDL_SetRenderTarget(game_screen.renderer, game_screen.backbuffer);

    SDL_SetRenderDrawColor(game_screen.renderer, red, green, blue, 0);
    SDL_RenderClear(game_screen.renderer);
    SDL_SetRenderDrawColor(game_screen.renderer, 255, 255, 255, 0);

    SDL_SetRenderTarget(game_screen.renderer, NULL);

}

void flip()
{
    SDL_RenderPresent(game_screen.renderer);
}

// draw the back-buffer to the visible window
void draw_backbuffer()
{
    SDL_Rect dest;
    dest.x = 0;
    dest.y = 0;
    dest.w = game_screen.width * game_screen.scale;
    dest.h = game_screen.height * game_screen.scale;

    SDL_RenderCopy(game_screen.renderer, game_screen.backbuffer, NULL, &dest);

}

// draws a sprite to the back-buffer
void draw_sprite(Sprite *spr)
{
    if (!spr->active) return;

    SDL_Rect src, dest;

    src.x = spr->source_x;
    src.y = spr->source_y;
    src.w = spr->width;
    src.h = spr->height;

    dest.x = (int) spr->x;
    dest.y = (int) spr->y;
    dest.w = spr->width;
    dest.h = spr->height;

    SDL_SetRenderTarget(game_screen.renderer, game_screen.backbuffer);
    SDL_RenderCopy(game_screen.renderer, spr->texture, &src, &dest);
    SDL_SetRenderTarget(game_screen.renderer, NULL);
}

void draw_subimage_rect(SDL_Texture *texture, int x, int y, int width, int height, int source_x, int source_y)
{
    SDL_Rect src, dest;

    src.x = source_x;
    src.y = source_y;
    src.w = width;
    src.h = height;

    dest.x = x;
    dest.y = y;
    dest.w = width;
    dest.h = height;

    SDL_SetRenderTarget(game_screen.renderer, game_screen.backbuffer);
    SDL_RenderCopy(game_screen.renderer, texture, &src, &dest);
    SDL_SetRenderTarget(game_screen.renderer, NULL);
}

// load an BMP-image file and converts it to a SDL-Texture
SDL_Texture* load_bmp(char *filename)
{
    SDL_Texture *texture;
    SDL_Surface *surface;

    surface = SDL_LoadBMP(filename);
    if (surface == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                    "error loading image",
                                    "error on image loading!",
                                    game_screen.window);
        return NULL;
    }

    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 255, 0, 255));
    texture = SDL_CreateTextureFromSurface(game_screen.renderer, surface);

    if (texture == NULL)
    {
        if (surface != NULL) SDL_FreeSurface(surface);
        return NULL;
    }

    if (surface != NULL) SDL_FreeSurface(surface);
    return texture;
}

// removes a loaded image file from memory
void unload_bmp(SDL_Texture *texture)
{
    if (texture != NULL) SDL_DestroyTexture(texture);
}

// ############################################################################
// GAME-INPUT FUNCTIONS
// ############################################################################
void check_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                check_keyboard(event.key.keysym.sym, true);
                break;

            case SDL_KEYUP:
                check_keyboard(event.key.keysym.sym, false);
                break;
        }
    }
}

// KEYBOARD HANDLING
void check_keyboard(int keycode, bool flag)
{
    switch(keycode)
    {
        case SDLK_ESCAPE:
            game_input.back = flag;
            break;

        case SDLK_LEFT:
            game_input.left = flag;
            break;

        case SDLK_RIGHT:
            game_input.right = flag;
            break;

        case SDLK_UP:
            game_input.up = flag;
            break;

        case SDLK_DOWN:
            game_input.down = flag;
            break;

        case SDLK_d:
            game_input.fire1 = flag;
            break;

        case SDLK_f:
            game_input.fire2 = flag;
            break;
    }
}

void reset_input()
{
    game_input.back = false;
    game_input.start = false;
    game_input.left = false;
    game_input.right = false;
    game_input.up = false;
    game_input.down = false;
    game_input.fire1 = false;
    game_input.fire2 = false;
}

bool keydown_back()
{
    return game_input.back;
}

bool keydown_left()
{
    return game_input.left;
}

bool keydown_right()
{
    return game_input.right;
}

bool keydown_up()
{
    return game_input.up;
}

bool keydown_down()
{
    return game_input.down;
}

bool keydown_fire1()
{
    return game_input.fire1;
}

bool keydown_fire2()
{
    return game_input.fire2;
}

// ############################################################################
// GAME-SPRITE FUNCTIONS
// ############################################################################
// when "texture" is NULL the texture will not be set!
void sprite_init(Sprite *spr, float x, float y, int width, int height, SDL_Texture *texture, int source_x, int source_y)
{
    spr->active = true;
    spr->x = x;
    spr->y = y;
    spr->width = width;
    spr->height = height;
    if (texture != NULL) spr->texture = texture;
    spr->source_x = source_x;
    spr->source_y = source_y;
}

void sprite_set_speed(Sprite *spr, float x, float y)
{
    spr->speed_x = x;
    spr->speed_y = y;
}

// ############################################################################
// GAME-TIMING FUNCTIONS
// ############################################################################
void delta_time_init()
{
    delta_time_last = SDL_GetTicks();
}

void delta_time_update()
{
    delta_time = (float)(SDL_GetTicks() - delta_time_last) / 1000.0f;
    delta_time_last = SDL_GetTicks();
}

// ############################################################################
// GAME-TILEMAP FUNCTIONS
// ############################################################################
void tilemap_init(int width, int height, SDL_Texture *texture)
{
    // current position of the tilemap in pixel
    tilemap.x = 0.0f;
    tilemap.y = 0.0f;

    // size of the tilemap
    tilemap.width = width;
    tilemap.height = height;

    // tile sheet of the tilemap
    tilemap.texture = texture;

    // size of a tile
    tilemap.tilesize = 16;

    // size of the tilesheet
    tilemap.tilesheet_width = 256;
    tilemap.tilesheet_height = 256;

    // calculate the position for each tile on the tilesheet
    int c = 0;
    for (int y = 0; y < tilemap.tilesheet_height / tilemap.tilesize; y++)
    {
        for (int x = 0; x < tilemap.tilesheet_width / tilemap.tilesize; x++)
        {
           tilemap.tile_postab[c] = x * tilemap.tilesize;
           tilemap.tile_postab[c+1] = y * tilemap.tilesize;
           c += 2;
        }
    }

}

bool tilemap_load(const char *filename)
{
    return true;
}

void tilemap_draw()
{
    int tile_id = 2;
    int tile_x, tile_y;

    int soft_scroll_x = (int) tilemap.x % tilemap.tilesize;

    for (int y = 0; y < game_screen.height / tilemap.tilesize; y++)
    {
        for (int x = 0; x < game_screen.width / tilemap.tilesize + 1; x++)
        {
            tile_x = tilemap.tile_postab[tile_id * 2];
            tile_y = tilemap.tile_postab[tile_id * 2 + 1];

            draw_subimage_rect(tilemap.texture,
                               (x * tilemap.tilesize) - soft_scroll_x,
                               y * tilemap.tilesize, tilemap.tilesize,
                               tilemap.tilesize,
                               tile_x, tile_y);
        }
    }
}
