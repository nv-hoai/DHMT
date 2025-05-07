#include <GL/glut.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include "delaunator.hpp"

// Global variables
std::vector<double> points; // x0, y0, z0, x1, y1, z1, ...
delaunator::Delaunator* triangulation = nullptr;
float rotationX = 0.0f;
float rotationY = 0.0f;
float zoom = 1.0f;
int lastMouseX = 0;
int lastMouseY = 0;
bool mouseLeftDown = false;
bool wireframeMode = false;

// Function to read points from a file
bool readPointsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    points.clear();
    float x, y, z;
    while (file >> x >> y >> z) {
        points.push_back(x);
        points.push_back(y);
        // For 2D triangulation, we ignore z for triangulation but keep it for visualization
        points.push_back(z);
    }

    file.close();
    return !points.empty();
}

// Function to normalize points to fit in [-1, 1] range
void normalizePoints() {
    if (points.empty()) return;

    // Find min and max values for x and y
    float minX = points[0], maxX = points[0];
    float minY = points[1], maxY = points[1];
    float minZ = points[2], maxZ = points[2];

    for (size_t i = 0; i < points.size(); i += 3) {
        minX = std::min(minX, (float)points[i]);
        maxX = std::max(maxX, (float)points[i]);
        minY = std::min(minY, (float)points[i + 1]);
        maxY = std::max(maxY, (float)points[i + 1]);
        minZ = std::min(minZ, (float)points[i + 2]);
        maxZ = std::max(maxZ, (float)points[i + 2]);
    }

    // Calculate the scale factors
    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    float rangeZ = maxZ - minZ;
    float scale = std::max(std::max(rangeX, rangeY), rangeZ) / 2.0f;

    // Normalize points
    for (size_t i = 0; i < points.size(); i += 3) {
        points[i] = (points[i] - (minX + maxX) / 2) / scale;
        points[i + 1] = (points[i + 1] - (minY + maxY) / 2) / scale;
        points[i + 2] = (points[i + 2] - (minZ + maxZ) / 2) / scale;
    }
}

// Function to generate Delaunay triangulation
void generateTriangulation() {
    if (points.empty()) return;

    // Create a 2D projection of points (x,y) for triangulation
    std::vector<double> coords;
    for (size_t i = 0; i < points.size(); i += 3) {
        coords.push_back(points[i]);     // x
        coords.push_back(points[i + 1]); // y
    }

    // Perform Delaunay triangulation
    delete triangulation;  // Delete previous triangulation if any
    triangulation = new delaunator::Delaunator(coords);

    std::cout << "Generated " << triangulation->triangles.size() / 3 << " triangles" << std::endl;
}

// GLUT display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Apply zoom and rotation
    glTranslatef(0.0f, 0.0f, -5.0f * zoom);
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);

    if (triangulation) {
        // Draw triangles
        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            // Set line color to black for wireframe mode
            glDisable(GL_LIGHTING);
            glColor3f(0.0f, 0.0f, 0.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            // Enable lighting for filled mode
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
            
            // Set up light position
            GLfloat lightPos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
            glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        }

        for (size_t i = 0; i < triangulation->triangles.size(); i += 3) {
            size_t i1 = triangulation->triangles[i];
            size_t i2 = triangulation->triangles[i + 1];
            size_t i3 = triangulation->triangles[i + 2];

            // Get 3D point indices
            size_t p1 = i1 * 3;
            size_t p2 = i2 * 3;
            size_t p3 = i3 * 3;

            // Calculate normal vector for lighting
            GLfloat v1[3] = { (float)(points[p2] - points[p1]),
                             (float)(points[p2 + 1] - points[p1 + 1]),
                             (float)(points[p2 + 2] - points[p1 + 2]) };
                             
            GLfloat v2[3] = { (float)(points[p3] - points[p1]),
                             (float)(points[p3 + 1] - points[p1 + 1]),
                             (float)(points[p3 + 2] - points[p1 + 2]) };
                             
            GLfloat normal[3] = {
                v1[1] * v2[2] - v1[2] * v2[1],
                v1[2] * v2[0] - v1[0] * v2[2],
                v1[0] * v2[1] - v1[1] * v2[0]
            };
            
            // Normalize
            float len = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
            if (len > 0) {
                normal[0] /= len;
                normal[1] /= len;
                normal[2] /= len;
            }

            // Draw triangle
            glBegin(GL_TRIANGLES);
                glNormal3fv(normal);
                
                // Use different colors based on height (z-value)
                GLfloat color1[4] = { 
                    0.2f, 
                    static_cast<GLfloat>(0.5f + points[p1 + 2] * 0.5f), 
                    static_cast<GLfloat>(0.8f - points[p1 + 2] * 0.5f), 
                    1.0f 
                };
                GLfloat color2[4] = { 
                    0.2f, 
                    static_cast<GLfloat>(0.5f + points[p2 + 2] * 0.5f), 
                    static_cast<GLfloat>(0.8f - points[p2 + 2] * 0.5f), 
                    1.0f 
                };
                GLfloat color3[4] = { 
                    0.2f, 
                    static_cast<GLfloat>(0.5f + points[p3 + 2] * 0.5f), 
                    static_cast<GLfloat>(0.8f - points[p3 + 2] * 0.5f), 
                    1.0f 
                };
                
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color1);
                glVertex3f(points[p1], points[p1 + 1], points[p1 + 2]);
                
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color2);
                glVertex3f(points[p2], points[p2 + 1], points[p2 + 2]);
                
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color3);
                glVertex3f(points[p3], points[p3 + 1], points[p3 + 2]);
            glEnd();
        }

        // Draw points
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        for (size_t i = 0; i < points.size(); i += 3) {
            glVertex3f(points[i], points[i + 1], points[i + 2]);
        }
        glEnd();
    }

    glutSwapBuffers();
}

// GLUT reshape function
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

// Handle mouse events
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseLeftDown = true;
            lastMouseX = x;
            lastMouseY = y;
        } else {
            mouseLeftDown = false;
        }
    }
}

// Handle mouse motion
void motion(int x, int y) {
    if (mouseLeftDown) {
        rotationY += (x - lastMouseX) * 0.5f;
        rotationX += (y - lastMouseY) * 0.5f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

// Handle keyboard events
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC key
            exit(0);
            break;
        case 'w':
        case 'W':
            wireframeMode = !wireframeMode;
            glutPostRedisplay();
            break;
        case '+':
            zoom *= 0.9f;
            glutPostRedisplay();
            break;
        case '-':
            zoom *= 1.1f;
            glutPostRedisplay();
            break;
    }
}

// Print usage information
void printUsage() {
    std::cout << "Delaunay Triangulation Visualizer" << std::endl;
    std::cout << "---------------------------" << std::endl;
    std::cout << "Usage: ./delaunay_visualizer <points_file>" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left Mouse Button: Rotate view" << std::endl;
    std::cout << "  W: Toggle wireframe mode" << std::endl;
    std::cout << "  +: Zoom in" << std::endl;
    std::cout << "  -: Zoom out" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;
}

int main(int argc, char** argv) {
    printUsage();
    
    // Check for input file
    if (argc < 2) {
        std::cout << "No input file specified. Generating sample data." << std::endl;
        
        // Generate some sample points (a simple grid)
        const int gridSize = 10;
        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                float x = (float)i / gridSize * 2.0f - 1.0f;
                float y = (float)j / gridSize * 2.0f - 1.0f;
                float z = 0.5f * sin(x * 3.0f) * cos(y * 3.0f); // Add some height variation
                
                points.push_back(x);
                points.push_back(y);
                points.push_back(z);
            }
        }
    } else {
        // Try to read points from file
        if (!readPointsFromFile(argv[1])) {
            std::cerr << "Failed to load points from file. Exiting." << std::endl;
            return 1;
        }
    }

    // Normalize points
    normalizePoints();
    
    // Generate triangulation
    generateTriangulation();

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Delaunay Triangulation Visualizer");

    // Register GLUT callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    // Set up OpenGL state
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Set background to white
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
    // Start the main loop
    glutMainLoop();

    // Clean up
    delete triangulation;
    
    return 0;
}