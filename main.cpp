#include <vector>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

Model *model        = NULL;

const int width  = 800;
const int height = 800;

Vec3f light_dir0(1,1,1);
Vec3f light_dir;
Vec3f       eye(0,0,3);
Vec3f    center(0,0,0);
Vec3f        up(0,1,0);

struct Shader : public IShader {
    mat<2,3,float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<4,3,float> varying_tri; // triangle coordinates (clip coordinates), written by VS, read by FS
    mat<3,3,float> varying_nrm; // normal per vertex to be interpolated by FS
    mat<3,3,float> ndc_tri;     // triangle in normalized device coordinates

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<3>((Projection*ModelView).invert_transpose()*embed<4>(model->normal(iface, nthvert), 0.f)));
        Vec4f gl_Vertex = Projection*ModelView*embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Vertex);
        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex/gl_Vertex[3]));
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec3f bn = (varying_nrm*bar).normalize();
        Vec2f uv = varying_uv*bar;

        mat<3,3,float> A;
        A[0] = ndc_tri.col(1) - ndc_tri.col(0);
        A[1] = ndc_tri.col(2) - ndc_tri.col(0);
        A[2] = bn;

        mat<3,3,float> AI = A.invert();

        Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        mat<3,3,float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, bn);

        Vec3f n = (B*model->normal(uv)).normalize();

        float diff = std::max(0.f, n*light_dir);
        color = model->diffuse(uv)*diff;

        return false;
    }
};

double common_get_secs(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec + (1e-9 * ts.tv_nsec);
}

const double COMMON_FPS_GRANULARITY_S = 0.5;
double common_fps_last_time_s;
unsigned int common_fps_nframes;

void common_fps_init() {
    common_fps_nframes = 0;
    common_fps_last_time_s = common_get_secs();
}

void common_fps_update_and_print() {
    double dt, current_time_s;
    current_time_s = common_get_secs();
    common_fps_nframes++;
    dt = current_time_s - common_fps_last_time_s;
    if (dt > COMMON_FPS_GRANULARITY_S) {
        printf("FPS = %f\n", common_fps_nframes / dt);
        common_fps_last_time_s = current_time_s;
        common_fps_nframes = 0;
    }
}

int main(int argc, char** argv) {
    if (2>argc) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    /* SDL init. */
    SDL_Event event;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    SDL_Window *window = NULL;
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(
        width, height,
        0, &window, &renderer
    );
    IMG_Init(0);
    common_fps_init();

    int pitch;
    void *pixels = NULL;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, width, height);

    float *zbuffer = new float[width*height];
    viewport(width/8, height/8, width*3/4, height*3/4);
    model = new Model(argv[1]);
    float angle = 0.1;
    while (1) {
        lookat(eye, center, up);
        projection(-1.f/(eye-center).norm());
        light_dir = proj<3>((Projection*ModelView*embed<4>(light_dir0, 0.f))).normalize();
        for (int i=width*height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
        TGAImage frame(width, height, TGAImage::RGBA);
        // TODO: if this is done, it crashes. Why?
        //SDL_LockTexture(texture, NULL, (void**)&frame.data, &pitch);
        Shader shader;
        for (int i=0; i<model->nfaces(); i++) {
            for (int j=0; j<3; j++) {
                shader.vertex(i, j);
            }
            triangle(shader.varying_tri, shader, frame, zbuffer);
        }
        frame.flip_vertically();

        //frame.write_tga_file("framebuffer.tga");
        //texture = IMG_LoadTexture(renderer, "framebuffer.tga");
        //SDL_RenderCopy(renderer, texture, NULL, NULL);
        //SDL_RenderPresent(renderer);
        //SDL_DestroyTexture(texture);

        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, frame.buffer(), width * height * 4);
        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        Vec3f oldEye = eye;
        eye[0] = cos(angle) * oldEye[0] + -sin(angle) * oldEye[2];
        eye[2] = sin(angle) * oldEye[0] +  cos(angle) * oldEye[2];
        oldEye = eye;

        common_fps_update_and_print();
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
            break;
    }
    delete model;

    /* SDL deinit. */
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    delete [] zbuffer;
    return 0;
}

