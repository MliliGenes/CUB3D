#include "include/MLX42/MLX42.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TILE_SIZE 64
#define FOV 60
#define PI 3.14159265358979323846

typedef struct s_player
{
    char ** map;
    int size;
    float direction_angle;
    double x_pos;
    double y_pos;
    double reminder_x;
    double reminder_y;
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
    angle = fmod(angle ,2 * PI);
    if (angle < 0)
        angle = (2 * PI) + angle;
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
        "101111011111101111001",
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

void draw_line(mlx_image_t *img, int x0, int y0, int x1, int y1, int color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (1)
    {
        x0 < img->width && y0 < img->height ? mlx_put_pixel(img, x0, y0, color) : NULL;
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

int32_t ft_pixel(int32_t r, int32_t g, int32_t b, int32_t a)
{
    return (r << 24 | g << 16 | b << 8 | a);
}

double cast_single_ray_distance(char **map, double player_x, double player_y, double ray_dir_x, double ray_dir_y)
{
    // Convert to map coordinates
    double pos_x = player_x / TILE_SIZE;
    double pos_y = player_y / TILE_SIZE;
    
    // Current map position
    int map_x = (int)pos_x;
    int map_y = (int)pos_y;
    
    // Distance ray travels for each unit step
    double delta_dist_x = fabs(1.0 / ray_dir_x);
    double delta_dist_y = fabs(1.0 / ray_dir_y);
    
    // Step direction and initial distances
    int step_x, step_y;
    double side_dist_x, side_dist_y;
    
    if (ray_dir_x < 0) {
        step_x = -1;
        side_dist_x = (pos_x - map_x) * delta_dist_x;
    } else {
        step_x = 1;
        side_dist_x = (map_x + 1.0 - pos_x) * delta_dist_x;
    }
    
    if (ray_dir_y < 0) {
        step_y = -1;
        side_dist_y = (pos_y - map_y) * delta_dist_y;
    } else {
        step_y = 1;
        side_dist_y = (map_y + 1.0 - pos_y) * delta_dist_y;
    }
    
    // DDA loop
    int hit = 0;
    int side;
    
    while (hit == 0) {
        if (side_dist_x < side_dist_y) {
            side_dist_x += delta_dist_x;
            map_x += step_x;
            side = 0;
        } else {
            side_dist_y += delta_dist_y;
            map_y += step_y;
            side = 1;
        }
        
        // Check bounds and wall hit
        if (map[map_y] && map[map_y][map_x] && map[map_y][map_x] == '1') {
            hit = 1;
        }
    }
    
    double wall_dist;
    if (side == 0) {
        wall_dist = (map_x - pos_x + (1 - step_x) / 2) / ray_dir_x;
    } else {
        wall_dist = (map_y - pos_y + (1 - step_y) / 2) / ray_dir_y;
    }
    
    return wall_dist * TILE_SIZE;
}

void cast_fov_rays(t_player *player, char **map)
{
    memset(player->direction_ray->pixels, 0, 
          player->direction_ray->width * player->direction_ray->height * sizeof(int32_t));
    
    double player_x = player->img->instances->x + player->size / 2.0;
    double player_y = player->img->instances->y + player->size / 2.0;
    
    int num_rays = 60;
    double fov_radians = deg_to_radian(60);  // 60 degrees in radians
    double angle_step = fov_radians / num_rays;
    
    // Starting angle (left edge of FOV)
    double start_angle = player->direction_angle - (fov_radians /2);
    
    // Cast each ray
    for (int i = 0; i < num_rays; i++) {
        // Current ray angle
        double ray_angle = start_angle + (i * angle_step);
        ray_angle = normalize_angle(ray_angle);
        
        // Ray direction
        double ray_dir_x = cos(ray_angle);
        double ray_dir_y = sin(ray_angle);
        
        // Cast the ray and get distance
        double wall_dist = cast_single_ray_distance(map, player_x, player_y, ray_dir_x, ray_dir_y);
        
        // Calculate end point
        int end_x = (int)(player_x + ray_dir_x * wall_dist);
        int end_y = (int)(player_y + ray_dir_y * wall_dist);
        
        // Choose color based on ray (center ray red, others yellow)
        int color = (i == num_rays / 2) ? 0xFF0000FF : 0xFF0000FF;
        
        // Draw the ray
        draw_line(player->direction_ray, 
                 (int)player_x, (int)player_y, 
                 end_x, end_y, color);
    }
}

void move_player(void *param)
{
    t_player *player = (t_player *)param;
    mlx_t *mlx = player->mlx;

    int move_forward = 0;
    int move_sideways = 0;

    double rot_speed = PI/20;
    double move_speed = 2;

    if (mlx_is_key_down(mlx, MLX_KEY_ESCAPE))
        mlx_close_window(mlx);

    if (mlx_is_key_down(mlx, MLX_KEY_W) || mlx_is_key_down(mlx, MLX_KEY_UP))
        move_forward = move_speed;
    if (mlx_is_key_down(mlx, MLX_KEY_S) || mlx_is_key_down(mlx, MLX_KEY_DOWN))
        move_forward = -move_speed;

    if (mlx_is_key_down(mlx, MLX_KEY_A))
        move_sideways = -move_speed;
    if (mlx_is_key_down(mlx, MLX_KEY_D))
        move_sideways = move_speed;

    if (mlx_is_key_down(mlx, MLX_KEY_LEFT))
        player->direction_angle -= rot_speed;
    if (mlx_is_key_down(mlx, MLX_KEY_RIGHT))
        player->direction_angle += rot_speed;
    
    player->direction_angle = normalize_angle(player->direction_angle);

    double forward_x = cos(player->direction_angle) * move_forward;
    double forward_y = sin(player->direction_angle) * move_forward;
    
    float strafe_angle = player->direction_angle + PI/2;
    double strafe_x = cos(strafe_angle) * move_sideways;
    double strafe_y = sin(strafe_angle) * move_sideways;
    
    float total_x = forward_x + strafe_x + player->reminder_x;
    float total_y = forward_y + strafe_y + player->reminder_y;

    int move_x = (int)round(total_x);
    int move_y = (int)round(total_y);
    player->reminder_x = total_x - move_x;
    player->reminder_y = total_y - move_y;

    player->img->instances->x += move_x;
    player->img->instances->y += move_y;

    cast_fov_rays(player, player->map);
}

int main()
{
    char **map = create_dynamic_map();
    t_player player;
    player.size = 4;
    player.reminder_x = 0;
    player.reminder_y = 0;
    player.direction_angle = deg_to_radian(90);
    player.map = map;
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

    int player_center_x = start_x + player.size/2;
    int player_center_y = start_y + player.size/2;

    int end_x = player_center_x + cos(player.direction_angle) * 60;
    int end_y = player_center_y + sin(player.direction_angle) * 60;

    mlx_loop_hook(mlx, move_player, &player);
    mlx_loop(mlx);
    
    mlx_terminate(mlx);
    for (int i = 0; map[i]; i++)
        free(map[i]);
    free(map);
    
    return 0;
}