#include <MLX42/MLX42.h>
#include <MLX42/MLX42_Int.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


# define TILE_SIZE 32
# define FOV 60
# define PI 3.14159265

typedef struct s_player
{
    int x;
    int y;
    int size;
    float direction_angle;
    mlx_t *mlx;
    mlx_image_t *img;
    mlx_image_t *direction_ray;
    mlx_image_t *left_ray;
    mlx_image_t *right_ray;
}   t_player;

float deg_to_radian(float deg)
{
    return (deg * PI / 180);
}

char **create_dynamic_map(void)
{
    // Original static map
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
    int i = 0;
    int j = 0;

    while (i < TILE_SIZE - 1)
    {
        j = 0;
        while (j < TILE_SIZE - 1)
        {
            mlx_put_pixel(img, x + j, y + i, color);
            j++;
        }
        i++;
    }
}

void build_map(char **map, mlx_image_t *img)
{
    int i = 0;
    int j = 0;

    while (map[i])
    {
        j = 0;
        while (map[i][j])
        {
            if (map[i][j] == '1')
                draw_square(img, j *TILE_SIZE, i *TILE_SIZE, 0x101010FF);
            else
                draw_square(img, j *TILE_SIZE, i *TILE_SIZE, 0xA0A0A0FF);
            j++;
        }
        i++;
    }
}

void init_bg(mlx_image_t *bg)
{
    int i = 0;
    int j = 0;

    while (i < 10 * TILE_SIZE)
    {
        j = 0;
        while (j < 6 * TILE_SIZE)
        {
            draw_square(bg, i, j, 0x000000FF);
            j++;
        }
        i++;
    }
}

void move_player(mlx_key_data_t key, void *param)
{
    t_player *player = (t_player *)param;
    mlx_t *mlx = player->mlx;

    int old_x = player->img->instances->x;
    int old_y = player->img->instances->y;

    if (key.key == 27)
		mlx_close_window(mlx);
    // if ()
    //    old_y -= 50;
    // if ()
    //     old_y += 50;
    // if ()
    //     old_x -= 50;
    // if ()
    //     old_x += 50; 
    
    player->img->instances->x = old_x * 4;
    player->img->instances->y = old_y * 4;
}

int main()
{
    char **map = create_dynamic_map();
    
    t_player player;
    player.x = 5;
    player.y = 3;
    player.size = 4;
    player.direction_angle = deg_to_radian(90);

    int SCREEN_WIDTH = strlen(*map) * TILE_SIZE;
    int SCREEN_HEIGHT = 9 * TILE_SIZE;

    mlx_t* mlx = mlx_init(SCREEN_WIDTH, SCREEN_HEIGHT, "cub", false);
    if (!mlx)
        return (1);

    mlx_image_t* img = mlx_new_image(mlx, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!img)
        return (1);

   player.img = mlx_new_image(mlx, player.size, player.size);
    if (!player.img)
        return (1);
    
    mlx_image_t *bg = mlx_new_image(mlx, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!bg)
        return (1);

    player.direction_ray = mlx_new_image(mlx, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!player.direction_ray)
        return 1;
    player.left_ray = mlx_new_image(mlx, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!player.left_ray)
        return 1;
    player.right_ray = mlx_new_image(mlx, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!player.right_ray)
        return 1;

    init_bg(bg);
    mlx_image_to_window(mlx, bg, 0, 0);

    build_map(map, img);
    mlx_image_to_window(mlx, img, 0, 0);

    float player_center_x = player.x * TILE_SIZE + TILE_SIZE / 2;
    float player_center_y = player.y * TILE_SIZE + TILE_SIZE / 2;

    draw_square(player.img, 1, 1, 0x000000FF);

    float ray_length = 600.0f;
    float fov = deg_to_radian(FOV);
    float half_fov = fov / 2;

    float center_angle = player.direction_angle;
    float end_x = player_center_x + cos(center_angle) * ray_length;
    float end_y = player_center_y + sin(center_angle) * ray_length;
    draw_line(player.direction_ray, player_center_x, player_center_y, end_x, end_y, 0xFF0000FF);

    float left_angle = center_angle - half_fov;
    end_x = player_center_x + cos(left_angle) * ray_length;
    end_y = player_center_y + sin(left_angle) * ray_length;
    draw_line(player.left_ray, player_center_x, player_center_y, end_x, end_y, 0xFF0000FF);

    float right_angle = center_angle + half_fov;
    end_x = player_center_x + cos(right_angle) * ray_length;
    end_y = player_center_y + sin(right_angle) * ray_length;
    draw_line(player.right_ray, player_center_x, player_center_y, end_x, end_y, 0xFF0000FF);

    mlx_image_to_window(mlx, player.direction_ray, 0, 0);
    mlx_image_to_window(mlx, player.left_ray, 0, 0);
    mlx_image_to_window(mlx, player.right_ray, 0, 0);
    mlx_image_to_window(mlx, player.img, player_center_x - player.size / 2, player_center_y - player.size /2);
    
    mlx_loop(mlx);
    mlx_key_hook(mlx, move_player, &player);
    mlx_close_window(mlx);

    return (0);
}
