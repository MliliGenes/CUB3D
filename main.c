#include <MLX42/MLX42.h>
#include <MLX42/MLX42_Int.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TILE_SIZE 64
#define FOV 60
#define PI 3.14159265358979323846

typedef struct s_player
{
    int size;
    float direction_angle;
    mlx_t *mlx;
    mlx_image_t *img;
    mlx_image_t *direction_ray;
} t_player;

float deg_to_radian(float deg)
{
    return (deg * PI / 180);
}

float normalize_angle(float angle)
{
    angle = fmod(angle, 2 * PI);
    if (angle < 0)
        angle += 2 * PI;
    return angle;
}

char **create_dynamic_map(void)
{
    char *static_map[] = {
        "111111111111111111111",
        "100000000010000000001",
        "101111010010101111101",
        "101000010000100000101",
        "101011110111101110101",
        "101000000000000000101",
        "101111011111101111101",
        "100000000010000000001",
        "111111111111111111111",
        NULL
    };

    int rows = 0;
    while (static_map[rows] != NULL)
        rows++;

    char **dynamic_map = malloc((rows + 1) * sizeof(char *));
    if (!dynamic_map)
        return NULL;

    for (int i = 0; i < rows; i++) {
        dynamic_map[i] = strdup(static_map[i]);
        if (!dynamic_map[i]) {
            for (int j = 0; j < i; j++)
                free(dynamic_map[j]);
            free(dynamic_map);
            return NULL;
        }
    }
    dynamic_map[rows] = NULL;
    return dynamic_map;
}

void draw_line(void *img, int x0, int y0, int x1, int y1, int color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (1)
    {
        mlx_put_pixel(img, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_square(mlx_image_t *img, int x, int y, int color)
{
    for (int i = 0; i < TILE_SIZE - 1; i++) {
        for (int j = 0; j < TILE_SIZE - 1; j++) {
            mlx_put_pixel(img, x + j, y + i, color);
        }
    }
}

void build_map(char **map, mlx_image_t *img)
{
    for (int i = 0; map[i]; i++) {
        for (int j = 0; map[i][j]; j++) {
            draw_square(img, j * TILE_SIZE, i * TILE_SIZE, 
                       map[i][j] == '1' ? 0x101010FF : 0xA0A0A0FF);
        }
    }
}

void move_player(void *param)
{
    t_player *player = (t_player *)param;
    mlx_t *mlx = player->mlx;

    float move_forward = 0;
    float move_sideways = 0;
    float rot_speed = 0.02f;
    float move_speed = 2.0f; //-rate independent

    if (mlx_is_key_down(mlx, MLX_KEY_ESCAPE))
        mlx_close_window(mlx);

    // Movement inputs
    if (mlx_is_key_down(mlx, MLX_KEY_W))
        move_forward = move_speed;
    if (mlx_is_key_down(mlx, MLX_KEY_S))
        move_forward = -move_speed;

    if (mlx_is_key_down(mlx, MLX_KEY_A))
        move_sideways = -move_speed;
    if (mlx_is_key_down(mlx, MLX_KEY_D))
        move_sideways = move_speed;

    // Rotation inputs
    if (mlx_is_key_down(mlx, MLX_KEY_LEFT))
        player->direction_angle -= rot_speed;
    if (mlx_is_key_down(mlx, MLX_KEY_RIGHT))
        player->direction_angle += rot_speed;
    
    player->direction_angle = normalize_angle(player->direction_angle);

    // Calculate movement vectors
    float forward_x = cosf(player->direction_angle) * move_forward;
    float forward_y = sinf(player->direction_angle) * move_forward;
    
    float strafe_angle = player->direction_angle + PI/2.0f;
    float strafe_x = cosf(strafe_angle) * move_sideways;
    float strafe_y = sinf(strafe_angle) * move_sideways;

    // Update position (add to current position)
    player->img->instances->x += forward_x + strafe_x;
    player->img->instances->y += forward_y + strafe_y;

    // Update direction indicator
    memset(player->direction_ray->pixels, 0, 
          player->direction_ray->width * player->direction_ray->height * sizeof(int32_t));
    
    float end_x = player->img->instances->x + cosf(player->direction_angle) * 60.0f;
    float end_y = player->img->instances->y + sinf(player->direction_angle) * 60.0f;
    draw_line(player->direction_ray, 
             player->img->instances->x, 
             player->img->instances->y, 
             end_x, end_y, 0xFF0000FF);
}

int main()
{
    char **map = create_dynamic_map();
    t_player player;
    player.size = 4;
    player.direction_angle = deg_to_radian(0);

    int SCREEN_WIDTH = strlen(*map) * TILE_SIZE;
    int SCREEN_HEIGHT = 9 * TILE_SIZE;

    mlx_t* mlx = mlx_init(SCREEN_WIDTH, SCREEN_HEIGHT, "cub", false);
    if (!mlx)
        return 1;
    player.mlx = mlx;

    mlx_image_t* img = mlx_new_image(mlx, SCREEN_WIDTH, SCREEN_HEIGHT);
    build_map(map, img);
    mlx_image_to_window(mlx, img, 0, 0);

    int start_x = 5 * TILE_SIZE - player.size/2;
    int start_y = 3 * TILE_SIZE - player.size/2;
    player.img = mlx_new_image(mlx, player.size, player.size);
    for (int y = 0; y < player.size; y++) {
        for (int x = 0; x < player.size; x++) {
            mlx_put_pixel(player.img, x, y, 0xFF0000FF);
        }
    }
    mlx_image_to_window(mlx, player.img, start_x, start_y);

    player.direction_ray = mlx_new_image(mlx, SCREEN_WIDTH, SCREEN_HEIGHT);
    mlx_image_to_window(mlx, player.direction_ray, 0, 0);

    float player_center_x = start_x + player.size/2;
    float player_center_y = start_y + player.size/2;
    float end_x = player_center_x + cos(player.direction_angle) * 60;
    float end_y = player_center_y + sin(player.direction_angle) * 60;
    draw_line(player.direction_ray, player_center_x, player_center_y, end_x, end_y, 0xFF0000FF);

    mlx_loop_hook(mlx, move_player, &player);
    mlx_loop(mlx);
    
    mlx_terminate(mlx);
    for (int i = 0; map[i]; i++)
        free(map[i]);
    free(map);
    
    return 0;
}