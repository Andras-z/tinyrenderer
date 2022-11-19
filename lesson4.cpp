#include <vector>
#include <cmath>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

const int width  = 800;
const int height = 800;
const int depth  = 255;

Model *model = NULL;
int *zbuffer = NULL;
vec3 light_dir(0,0,-1);
vec3 camera(0,0,3);

vec3 m2v(Matrix m) {
    return vec3(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

Matrix v2m(vec3 v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix Matrix_viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

TGAColor sample2D_test(const TGAImage &img, vec2 &uvf) {
    return img.get(uvf[0] * img.width(), uvf[1] * img.height());
}

void triangle(vec3 t0, vec3 t1, vec3 t2, vec2 uv0, vec2 uv1, vec2 uv2, TGAImage &image, float intensity, int *zbuffer) {
    if (t0.y==t1.y && t0.y==t2.y) return; // i dont care about degenerate triangles
    if (t0.y>t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
    if (t0.y>t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
    if (t1.y>t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

    int total_height = t2.y-t0.y;
    for (int i=0; i<total_height; i++) {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height; // be careful: with above conditions no division by zero here
        vec3 A   =               t0  + vec3(t2-t0  )*alpha;
        vec3 B   = second_half ? t1  + vec3(t2-t1  )*beta : t0  + vec3(t1-t0  )*beta;
        vec2 uvA =               uv0 +      (uv2-uv0)*alpha;
        vec2 uvB = second_half ? uv1 +      (uv2-uv1)*beta : uv0 +      (uv1-uv0)*beta;
        if (A.x>B.x) { std::swap(A, B); std::swap(uvA, uvB); }
        for (int j=A.x; j<=B.x; j++) {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
            vec3   P = vec3(A) + vec3(B-A)*phi;
            vec2 uvP =     uvA +   (uvB-uvA)*phi;
            int idx = P.x+P.y*width;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
                TGAColor color = sample2D_test(model->diffuse(), uvP);
                image.set(P.x, P.y, TGAColor(color.bgra[2]*intensity, color.bgra[1]*intensity, color.bgra[0]*intensity));
            }
        }
    }
}

void Lesson4(void)
{
    model = new Model("../obj/african_head/african_head.obj");

    zbuffer = new int[width*height];
    for (int i=0; i<width*height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    { // draw the model
        Matrix Projection = Matrix::identity(4);
        Matrix ViewPort   = Matrix_viewport(width/8, height/8, width*3/4, height*3/4);
        Projection[3][2] = -1.f/camera.z;

        TGAImage image(width, height, TGAImage::RGB);
        for (int i=0; i<model->nfaces(); i++) {
            vec3 screen_coords[3];
            vec3 world_coords[3];
            for (int j=0; j<3; j++) {
                vec3 v = model->vert(i, j);
                screen_coords[j] =  m2v(ViewPort*Projection*v2m(v));
                world_coords[j]  = v;
            }
            vec3 n = cross((world_coords[2]-world_coords[0]), (world_coords[1]-world_coords[0]));
            n.normalize();
            float intensity = n*light_dir;
            if (intensity>0) {
                vec2 uv[3];
                for (int k=0; k<3; k++) {
                    uv[k] = model->uv(i, k);
                }
                triangle(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
            }
        }

        // image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("output_lesson4_head.tga");
    }

    { // dump z-buffer (debugging purposes only)
        TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
        for (int i=0; i<width; i++) {
            for (int j=0; j<height; j++) {
                std::uint8_t p = zbuffer[i+j * width];
                zbimage.set(i, j, TGAColor(&p, 1));
            }
        }
        // zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        zbimage.write_tga_file("zbuffer_lesson4.tga");
    }
    delete model;
    delete [] zbuffer;
    return;
}
