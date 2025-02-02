#include <math.h>
#include <X11/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <mlx.h>

#define WIDTH 800
#define HEIGHT 800

#define MIN_X -2
#define MAX_X 2
#define MIN_Y -2
#define MAX_Y 2

#define SCALING_FACTOR 1.05

typedef enum e_fractal {
    MANDELBROT,
    JULIA,
    BURNING_SHIP
} t_fractal;

typedef struct s_fractol {
    void *mlx_ptr;
    void *win_ptr;
    void *img_ptr;
    char *img_data;
    int bpp;
    int size_line;
    int endian;
    double scale;
    double offset_x;
    double offset_y;
    double c_re;
    double c_im;
    t_fractal fractal;
} t_fractol;

double map(double value, double min, double max, double new_min, double new_max) {
    return (value - min) / (max - min) * (new_max - new_min) + new_min;
}

int redraw(t_fractol* fractol) {
    int x, y;
    int color;
    int max_iter = 100;
    double p_re, p_im, c_re, c_im, z_re, z_im, z_re2, z_im2;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            p_re = map(x, 0, WIDTH, MIN_X, MAX_X) * fractol->scale + fractol->offset_x;
            p_im = map(y, 0, HEIGHT, MIN_Y, MAX_Y) * fractol->scale + fractol->offset_y;
            if (fractol->fractal == MANDELBROT || fractol->fractal == BURNING_SHIP) {
                c_re = p_re;
                c_im = p_im;
                z_re = 0;
                z_im = 0;
            } else if (fractol->fractal == JULIA) {
                c_re = fractol->c_re;
                c_im = fractol->c_im;
                z_re = p_re;
                z_im = p_im;
            }
            z_re2 = z_re * z_re;
            z_im2 = z_im * z_im;
            color = 0;
            while (z_re2 + z_im2 < 4 && color < max_iter) {
                if (fractol->fractal == MANDELBROT || fractol->fractal == JULIA) {
                    z_im = 2 * z_re * z_im + c_im;
                } else if (fractol->fractal == BURNING_SHIP) {
                    z_im = 2 * fabs(z_re) * fabs(z_im) + c_im;
                }
                z_re = z_re2 - z_im2 + c_re;
                z_re2 = z_re * z_re;
                z_im2 = z_im * z_im;
                color++;
            }
            *(unsigned int *)(fractol->img_data + (x * fractol->bpp / 8 + y * fractol->size_line)) = 0x0000FF * log10(color) / log10(max_iter);
        }
    }
    if (fractol->win_ptr) {
        mlx_put_image_to_window(fractol->mlx_ptr, fractol->win_ptr, fractol->img_ptr, 0, 0);
    }
    return 0;
}

int destroy(t_fractol *fractol)
{
    mlx_destroy_window(fractol->mlx_ptr, fractol->win_ptr);
    fractol->win_ptr = NULL;
    return 0;
}

int mouse_hook(int button, int x, int y, t_fractol *fractol) {
    if (button == 4) {
        fractol->offset_x += map(x, 0, WIDTH, MIN_X, MAX_X) * (1 - 1 * SCALING_FACTOR) * fractol->scale;
        fractol->offset_y += map(y, 0, HEIGHT, MIN_Y, MAX_Y) * (1 - 1 * SCALING_FACTOR) * fractol->scale;
        fractol->scale *= SCALING_FACTOR;
    } else if (button == 5) {
        fractol->offset_x += map(x, 0, WIDTH, MIN_X, MAX_X) * (1 - 1 / SCALING_FACTOR) * fractol->scale;
        fractol->offset_y += map(y, 0, HEIGHT, MIN_Y, MAX_Y) * (1 - 1 / SCALING_FACTOR) * fractol->scale;
        fractol->scale /= SCALING_FACTOR;
    }
    return 0;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

void print(const char *s) {
    while (*s) {
        write(1, s, 1);
        s++;
    }
}

double atod(const char *s) {
    double result = 0;
    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + *s - '0';
        s++;
    }
    if (*s == '.') {
        s++;
        double factor = 0.1;
        while (*s >= '0' && *s <= '9') {
            result += (*s - '0') * factor;
            factor *= 0.1;
            s++;
        }
    }
    return result * sign;
}

void parse_args(int argc, char **argv, t_fractol *fractol) {
    if (argc == 2) {
        if (strcmp(argv[1], "mandelbrot") == 0) {
            fractol->fractal = MANDELBROT;
        } else if (strcmp(argv[1], "burning_ship") == 0) {
            fractol->fractal = BURNING_SHIP;
        } else {
            print("Usage: ");
            print(argv[0]);
            print(" [mandelbrot|julia|burning_ship]\n");
            exit(1);
        }
    } else if (argc == 4) {
        if (strcmp(argv[1], "julia") == 0) {
            fractol->fractal = JULIA;
            fractol->c_re = atod(argv[2]);
            fractol->c_im = atod(argv[3]);
        } else {
            print("Usage: ");
            print(argv[0]);
            print(" julia <c_re> <c_im>\n");
            exit(1);
        }
    } else {
        print("Usage: ");
        print(argv[0]);
        print(" [mandelbrot|julia|burning_ship]\n");
        exit(1);
    }
}

int main(int argc, char **argv) {
    t_fractol fractol;
    fractol.scale = 1.0;

    parse_args(argc, argv, &fractol);

    fractol.mlx_ptr = mlx_init();
    fractol.win_ptr = mlx_new_window(fractol.mlx_ptr, WIDTH, HEIGHT, "fractol");
    fractol.img_ptr = mlx_new_image(fractol.mlx_ptr, WIDTH, HEIGHT);
    fractol.img_data = mlx_get_data_addr(fractol.img_ptr, &fractol.bpp, &fractol.size_line, &fractol.endian);

    mlx_hook(fractol.win_ptr, DestroyNotify, NoEventMask, destroy, &fractol);
    mlx_loop_hook(fractol.mlx_ptr, redraw, &fractol);
    mlx_mouse_hook(fractol.win_ptr, mouse_hook, &fractol);

    mlx_loop(fractol.mlx_ptr);

    mlx_destroy_image(fractol.mlx_ptr, fractol.img_ptr);
}