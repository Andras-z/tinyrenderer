#include <bits/stdc++.h>
#include "model.h"
#include "our_gl.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0,   255, 0,   255);
const TGAColor blue  = TGAColor(0,   0,   255, 255);

const int width  = 1000; 
const int height = 1000; 

static void line(vec2 v0, vec2 v1, TGAImage &image, TGAColor color)
{
    int x0 = v0.x;
    int y0 = v0.y;
    int x1 = v1.x;
    int y1 = v1.y; 
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            image.set(y, x, color);
        } else { 
            image.set(x, y, color);
        } 
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void rasterize(vec2 p0, vec2 p1, TGAImage &image, TGAColor color, int ybuffer[])
{
    if (p0.x>p1.x) {
        std::swap(p0, p1);
    }
    for (int x=p0.x; x<=p1.x; x++) {
        float t = (x-p0.x)/(float)(p1.x-p0.x);
        int y = p0.y*(1.-t) + p1.y*t;
        if (ybuffer[x]<y) {
            ybuffer[x] = y;
            image.set(x, 0, color);
        }
    }
}

vec3 barycentric(vec3 A, vec3 B, vec3 C, vec3 P) {
    vec3 s[2];
    for (int i=2; i--; ) {
        s[i][0] = C[i]-A[i];
        s[i][1] = B[i]-A[i];
        s[i][2] = A[i]-P[i];
    }
    vec3 u = cross(s[0], s[1]);
    if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return vec3(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    return vec3(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(vec3 *pts, float *zbuffer, TGAImage &image, TGAColor color) {
    vec2 bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    vec2 bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    vec2 clamp(image.width()-1, image.height()-1);
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0.f,      (float)std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    vec3 P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            vec3 bc_screen  = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            P.z = 0;
            for (int i=0; i<3; i++) P.z += pts[i][2]*bc_screen[i];
            if (zbuffer[int(P.x+P.y*width)]<P.z) {
                zbuffer[int(P.x+P.y*width)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}

vec3 world2screen(vec3 v) {
    return vec3(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

void Lesson3(void)
{
    TGAImage scene(width, height, TGAImage::RGB);

    // scene "2d mesh"
    line(vec2(20, 34),   vec2(744, 400), scene, red);
    line(vec2(120, 434), vec2(444, 400), scene, green);
    line(vec2(330, 463), vec2(594, 200), scene, blue);

    // screen line
    line(vec2(10, 10), vec2(790, 10), scene, white);

    // scene.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    scene.write_tga_file("output_lesson3.tga");

    TGAImage render(width, 16, TGAImage::RGB);
    int ybuffer[width];
    for (int i=0; i<width; i++) {
        ybuffer[i] = std::numeric_limits<int>::min();
    }

    rasterize(vec2(20, 34),   vec2(744, 400), render, red,   ybuffer);
    rasterize(vec2(120, 434), vec2(444, 400), render, green, ybuffer);
    rasterize(vec2(330, 463), vec2(594, 200), render, blue,  ybuffer);

    render.write_tga_file("output_lesson3_render.tga");

    TGAImage image(width, height, TGAImage::RGB);
    vec3 light_dir(0, 0, -1); // define light_dir
    Model model("../obj/african_head/african_head.obj");
    float *zbuffer = new float[width*height];
    for (int i=width*height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
    for (int i=0; i<model.nfaces(); i++) {
        vec3 pts[3];
        vec3 world_coords[3];
        for (int j=0; j<3; j++) {
            vec3 v = model.vert(i, j);
            pts[j] = world2screen(v);
            world_coords[j] = v;
        }
        vec3 n = cross((world_coords[2]-world_coords[0]), (world_coords[1]-world_coords[0])); 
        n.normalize();
        float intensity = n * light_dir;

        if (intensity > 0) {
            triangle(pts, zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        }
    }

    image.write_tga_file("output_lesson3_head.tga");
    return;
}
