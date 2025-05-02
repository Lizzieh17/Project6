//---------------------------------------
// Program: ray_trace.cpp
// Purpose: Assignement 6
// Author:  Lizzie Howell
// Date:   Spring 2025
//---------------------------------------
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#ifdef MAC
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
using namespace std;
#include "ray_classes.h"

#define XDIM 800
#define YDIM 800
#define MAX_FIXED_CYLINDERS 8
unsigned char image[YDIM][XDIM][3];
float position = 1;
float sphereAngle = 0.0;
float cylinderAngle = 5.0;
int num_fixed_cylinders = 0;
Cylinder3D fixed_cylinders[MAX_FIXED_CYLINDERS];

//---------------------------------------
// Generate a random float between min and max
//---------------------------------------
float myrand(float min, float max)
{
   return rand() * (max - min) / RAND_MAX + min;
}

//---------------------------------------
// Check if a point is in shadow
//---------------------------------------
bool in_shadow(Point3D pt, Vector3D dir, int current, Sphere3D sphere[], int scount, Cylinder3D cylinder, Cylinder3D fixed_cyls[])
{
   Point3D shadow_origin = pt;
   shadow_origin.px += 0.001 * dir.vx;
   shadow_origin.py += 0.001 * dir.vy;
   shadow_origin.pz += 0.001 * dir.vz;

   Ray3D shadow_ray;
   shadow_ray.set(shadow_origin, dir);

   Point3D point;
   Vector3D normal;

   // Check center sphere and orbiting sphere
   for (int i = 0; i < scount; i++)
   {
      if ((current != i) && sphere[i].get_intersection(shadow_ray, point, normal))
      {
         Vector3D offset;
         offset.set(point.px - shadow_origin.px,
                    point.py - shadow_origin.py,
                    point.pz - shadow_origin.pz);
         if (offset.dot(dir) > 0)
            return true;
      }
   }

   // Check orbiting cylinder
   if (current != 2 && cylinder.get_intersection(shadow_ray, point, normal))
   {
      Vector3D offset;
      offset.set(point.px - shadow_origin.px,
                 point.py - shadow_origin.py,
                 point.pz - shadow_origin.pz);
      if (offset.dot(dir) > 0)
         return true;
   }

   // Check fixed cylinders
   for (int i = 0; i < num_fixed_cylinders; i++)
   {
      if (current != (3 + i) && fixed_cyls[i].get_intersection(shadow_ray, point, normal))
      {
         Vector3D offset;
         offset.set(point.px - shadow_origin.px,
                    point.py - shadow_origin.py,
                    point.pz - shadow_origin.pz);
         if (offset.dot(dir) > 0)
            return true;
      }
   }

   return false;
}
//---------------------------------------
// Ray trace function
//---------------------------------------
void ray_trace()
{
   Point3D camera;
   camera.set(0, 0, -position);

   // create light source
   ColorRGB light_color;
   light_color.set(255, 255, 255);
   Vector3D light_dir;
   light_dir.set(-0.5, -0.5, -1.5);
   light_dir.normalize();

   // create center sphere
   Sphere3D center_sphere;
   Point3D center_pos;
   center_pos.set(0, 0, 5);
   center_sphere.set(center_pos, 1.0);

   // create orbiting sphere
   Sphere3D orbiting_sphere;
   float angleRad = sphereAngle * M_PI / 180.0;
   Point3D orbit_pos;
   orbit_pos.set(center_pos.px + 2.0 * cos(angleRad), -0.75, center_pos.pz + 2.0 * sin(angleRad));
   orbiting_sphere.set(orbit_pos, 1.0);

   // Create forbiting cylinder
   Cylinder3D cylinder;
   float cylRad = cylinderAngle * M_PI / 180.0;
   Point3D cylinder_pos;
   cylinder_pos.set(center_pos.px + 4.0 * cos(cylRad), -0.5, center_pos.pz + 4.0 * sin(cylRad)); // position cylinder in orbit
   Vector3D cylinder_axis;
   cylinder_axis.set(0, -0.75, 0);
   cylinder.set(cylinder_pos, cylinder_axis, 0.5, 1.0);

   for (int y = 0; y < YDIM; y++)
      for (int x = 0; x < XDIM; x++)
      {
         image[y][x][0] = image[y][x][1] = image[y][x][2] = 0;

         float xpos = (x - XDIM / 2) * 2.0 / XDIM;
         float ypos = (y - YDIM / 2) * 2.0 / YDIM;
         Point3D pixel;
         pixel.set(xpos, ypos, 0);

         Ray3D ray;
         ray.set(camera, pixel);

         Point3D p1, p2, p3, pf; // intersection points for orbiting sphere, center sphere, and orbiting cylinder
         Vector3D n1, n2, n3, nf; // normals for orbiting sphere, center sphere, and orbiting cylinder

         // Check for intersection with orbiting sphere, center sphere, and orbiting cylinder
         bool hit1 = orbiting_sphere.get_intersection(ray, p1, n1);
         bool hit2 = center_sphere.get_intersection(ray, p2, n2);
         bool hit3 = cylinder.get_intersection(ray, p3, n3);

         // check for intersection with fixed cylinders
         bool hit_fixed = false;
         int fixed_hit_index = -1;
         for (int i = 0; i < num_fixed_cylinders; i++)
         {
            Point3D pt;
            Vector3D nt;
            if (fixed_cylinders[i].get_intersection(ray, pt, nt))
            {
               if (!hit_fixed || pt.distance(camera) < pf.distance(camera))
               {
                  pf = pt;
                  nf = nt;
                  hit_fixed = true;
                  fixed_hit_index = i;
               }
            }
         }

         Point3D final_p;
         Vector3D final_n;
         ColorRGB final_color;
         bool hit = false;
         int current_object = -1;

         // Determine which object was hit based on distance from camera
         // and assign final color
         // 0 - orbiting sphere, 1 - center sphere, 2 - orbiting cylinder, 3+ - fixed cylinders
         if (hit1 && (!hit2 || p1.distance(camera) < p2.distance(camera)) &&
             (!hit3 || p1.distance(camera) < p3.distance(camera)) &&
             (!hit_fixed || p1.distance(camera) < pf.distance(camera)))
         {
            final_p = p1;
            final_n = n1;
            final_color.set(255, 255, 255); // white
            current_object = 0;
            hit = true;
         }
         else if (hit2 && (!hit3 || p2.distance(camera) < p3.distance(camera)) &&
                  (!hit_fixed || p2.distance(camera) < pf.distance(camera)))
         {
            final_p = p2;
            final_n = n2;
            final_color.set(0, 0, 200); // blue
            current_object = 1;
            hit = true;
         }
         else if (hit3 && (!hit_fixed || p3.distance(camera) < pf.distance(camera)))
         {
            final_p = p3;
            final_n = n3;
            final_color.set(0, 200, 0); // green
            current_object = 2;
            hit = true;
         }
         else if (hit_fixed)
         {
            final_p = pf;
            final_n = nf;
            final_color.set(255, 150, 200); // pink for fixed cylinders
            current_object = 3 + fixed_hit_index;
            hit = true;
         }

         if (hit)
         {
            Phong shader;
            shader.SetCamera(camera);
            shader.SetLight(light_color, light_dir);
            shader.SetObject(final_color, 0.3, 0.4, 0.3, 10);

            Sphere3D spheres[2] = {orbiting_sphere, center_sphere};
            bool shadow = in_shadow(final_p, light_dir, current_object, spheres, 2, cylinder, fixed_cylinders);

            if (shadow)
            {
               final_color.R *= 0.3;
               final_color.G *= 0.3;
               final_color.B *= 0.3;
            }
            else
            {
               shader.GetShade(final_p, final_n, final_color);
            }

            image[y][x][0] = final_color.R;
            image[y][x][1] = final_color.G;
            image[y][x][2] = final_color.B;
         }
      }
}

//--------------------------------------
// Idle function to animate
//---------------------------------------
void idle()
{
   sphereAngle += 2.0;
   cylinderAngle += 5.0;
   if (sphereAngle > 360)
      sphereAngle -= 360;
   if (cylinderAngle > 360)
      cylinderAngle -= 360;

   ray_trace();
   glutPostRedisplay();
}

//---------------------------------------
// Initialize fixed cylinders with random positions and dimensions
//---------------------------------------
void init_fixed_cylinders(){
   num_fixed_cylinders = myrand(2, MAX_FIXED_CYLINDERS);
   for (int i = 0; i < num_fixed_cylinders; i++)
   {
      float x = myrand(-4, 4);
      float y = myrand(-1, 1);
      float z = myrand(-4, 4);
      Point3D pos;
      pos.set(x, y, z);
      Vector3D axis;
      axis.set(0, -1, 0);
      float radius = myrand(0.2, 0.5);
      float height = myrand(0.5, 1.5);
      fixed_cylinders[i].set(pos, axis, radius, height);
   }
}

//---------------------------------------
// Init function for OpenGL
//---------------------------------------
void init()
{
   glClearColor(0.0, 0.0, 0.0, 1.0);

   cout << "Program commands:\n"
        << "   '+' - increase camera distance\n"
        << "   '-' - decrease camera distance\n"
        << "   'q' - quit program\n";
   
   init_fixed_cylinders();
   ray_trace();
}

//---------------------------------------
// Display callback for OpenGL
//---------------------------------------
void display()
{
   glClear(GL_COLOR_BUFFER_BIT);
   glDrawPixels(XDIM, YDIM, GL_RGB, GL_UNSIGNED_BYTE, image);
   glFlush();
}

//---------------------------------------
// Keyboard callback for OpenGL
//---------------------------------------
void keyboard(unsigned char key, int x, int y)
{
   if (key == 'q')
      exit(0);
   else if (key == '+' && position < 5)
      position *= 1.1;
   else if (key == '-' && position > 1)
      position /= 1.1;

   ray_trace();
   glutPostRedisplay();
}

//---------------------------------------
// Main program
//---------------------------------------
int main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(XDIM, YDIM);
   glutInitWindowPosition(0, 0);
   glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
   glutCreateWindow("Ray Trace");
   srand(time(NULL));
   init();

   glutDisplayFunc(display);
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);
   glutMainLoop();
   return 0;
}
