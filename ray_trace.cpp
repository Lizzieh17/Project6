//---------------------------------------
// Program: ray_trace.cpp
// Purpose: Ray tracing with orbiting sphere around center
// Author:  John Gauch + Modified
// Date:    Modified April 2025
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
// Include ray tracing and phong shading code
#include "ray_classes.h"

// Global variables
#define XDIM 800
#define YDIM 800
unsigned char image[YDIM][XDIM][3];
float position = 1;
float sphereAngle = 0.0;
float cylinderAngle = 5.0;
//---------------------------------------
// Calculate random value between [min..max]
//---------------------------------------
float myrand(float min, float max)
{
   return rand() * (max - min) / RAND_MAX + min;
}

//---------------------------------------
// Check to see if point is in shadow
//---------------------------------------
bool in_shadow(Point3D pt, Vector3D dir, int current, Sphere3D sphere[], int count, Cylinder3D *cylinder)
{
   // Offset ray origin slightly to avoid self-shadowing
   Point3D shadow_origin = pt;
   shadow_origin.px += 0.001 * dir.vx;
   shadow_origin.py += 0.001 * dir.vy;
   shadow_origin.pz += 0.001 * dir.vz;

   Ray3D shadow_ray;
   shadow_ray.set(shadow_origin, dir);

   Point3D point;
   Vector3D normal;

   // Check all spheres except the current one
   for (int i = 0; i < count; i++)
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

   // Check the cylinder if it's not the current object
   if (cylinder != nullptr && current != 2 && cylinder->get_intersection(shadow_ray, point, normal))
   {
      Vector3D offset;
      offset.set(point.px - shadow_origin.px,
                 point.py - shadow_origin.py,
                 point.pz - shadow_origin.pz);
      if (offset.dot(dir) > 0)
         return true;
   }

   return false;
}

//---------------------------------------
// Ray tracing function
//---------------------------------------
void ray_trace()
{
   // Define camera point
   Point3D camera;
   camera.set(0, 0, -position);

   // Define light source
   ColorRGB light_color;
   light_color.set(255, 255, 255);
   Vector3D light_dir;
   light_dir.set(-0.5, -0.5, -1.5);
   light_dir.normalize();

   // Define central sphere
   Sphere3D center_sphere;
   Point3D center_pos;
   center_pos.set(0, 0, 5);
   float center_radius = 1.0;
   center_sphere.set(center_pos, center_radius);

   // Define orbiting sphere
   Sphere3D orbiting_sphere;
   float orbit_radius = 2.0;
   float orbit_sphere_radius = 1.0;
   float angleRad = sphereAngle * M_PI / 180.0;
   float x = center_pos.px + orbit_radius * cos(angleRad);
   float z = center_pos.pz + orbit_radius * sin(angleRad);
   Point3D orbit_pos;
   orbit_pos.set(x, -0.75, z);
   orbiting_sphere.set(orbit_pos, orbit_sphere_radius);

   // Define orbiting cylinder
   Cylinder3D cylinder;
   float cyl_orbit_radius = 4.0;
   float cylRad = cylinderAngle * M_PI / 180.0;
   float cyl_x = center_pos.px + cyl_orbit_radius * cos(cylRad);
   float cyl_z = center_pos.pz + cyl_orbit_radius * sin(cylRad);
   Point3D cylinder_pos;
   cylinder_pos.set(cyl_x, -0.5, cyl_z);
   Vector3D cylinder_axis;
   cylinder_axis.set(0, -0.75, 0);
   float cylinder_radius = 0.5;
   float cylinder_height = 1.0;
   cylinder.set(cylinder_pos, cylinder_axis, cylinder_radius, cylinder_height);

   // Loop over pixels
   for (int y = 0; y < YDIM; y++)
      for (int x = 0; x < XDIM; x++)
      {
         image[y][x][0] = 0;
         image[y][x][1] = 0;
         image[y][x][2] = 0;

         float xpos = (x - XDIM / 2) * 2.0 / XDIM;
         float ypos = (y - YDIM / 2) * 2.0 / YDIM;
         Point3D pixel;
         pixel.set(xpos, ypos, 0);

         Ray3D ray;
         ray.set(camera, pixel);

         // Try all objects
         Point3D p1, p2, p3;
         Vector3D n1, n2, n3;
         bool hit1 = orbiting_sphere.get_intersection(ray, p1, n1);
         bool hit2 = center_sphere.get_intersection(ray, p2, n2);
         bool hit3 = cylinder.get_intersection(ray, p3, n3);

         Point3D final_p;
         Vector3D final_n;
         ColorRGB final_color;
         bool hit = false;
         int current_object = -1;

         if (hit1 && (!hit2 || p1.distance(camera) < p2.distance(camera)) &&
             (!hit3 || p1.distance(camera) < p3.distance(camera)))
         {
            final_p = p1;
            final_n = n1;
            final_color.set(255, 255, 255); // white
            current_object = 0;
            hit = true;
         }
         else if (hit2 && (!hit3 || p2.distance(camera) < p3.distance(camera)))
         {
            final_p = p2;
            final_n = n2;
            final_color.set(0, 0, 200); // blue
            current_object = 1;
            hit = true;
         }
         else if (hit3)
         {
            final_p = p3;
            final_n = n3;
            final_color.set(0, 200, 0); // green
            current_object = 2;
            hit = true;
         }

         if (hit)
         {
            Phong shader;
            shader.SetCamera(camera);
            shader.SetLight(light_color, light_dir);
            shader.SetObject(final_color, 0.3, 0.4, 0.3, 10);

            Sphere3D spheres[2];
            spheres[0] = orbiting_sphere;
            spheres[1] = center_sphere;

            bool shadow = in_shadow(final_p, light_dir, current_object, spheres, 2, &cylinder);
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
// Init function for OpenGL
//---------------------------------------
void init()
{
   glClearColor(0.0, 0.0, 0.0, 1.0);

   cout << "Program commands:\n"
        << "   '+' - increase camera distance\n"
        << "   '-' - decrease camera distance\n"
        << "   'q' - quit program\n";

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
   init();

   glutDisplayFunc(display);
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);
   glutMainLoop();
   return 0;
}
