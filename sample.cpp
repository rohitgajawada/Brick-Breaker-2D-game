#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>   
#include <map>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ao/ao.h>
#include <mpg123.h>

#define BITS 8

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct COLOR {
    float r;
    float g;
    float b;
};
typedef struct COLOR color;
COLOR red = {1, 0, 0};
COLOR green = {0, 1, 0};
COLOR black = {30 / 255.0, 30 / 255.0, 21 / 255.0};
COLOR brown = {95/255.0, 63/255.0, 32/255.0};
COLOR coolblue = {0/255.0, 76/255.0, 153/255.0};
COLOR silver = {192/255.0, 192/255.0, 192/255.0};
COLOR yellow = {1, 1, 0};
color teal = {204/255.0, 102/255.0, 0/255.0};
color lblue = {51/255.0, 153/255.0, 255/255.0};

struct Sprite {
    string name;
    int exist;
    COLOR color;
    float x, y;
    float angle;
    float height, width;
    VAO* object;
    string mirname;
};

typedef struct Sprite Sprite;

map <string, Sprite> bricks;
map <string, Sprite> baskets;
map <string, Sprite> cannons;
map <string, Sprite> mirrors;
map <string, Sprite> bullets;
map <string, Sprite> bgs;
map <string, Sprite> scorebd;

GLuint programID;
int numblocks = 0;
int numbullets = 0;
int score = 50;
int health = 10;
double current_time;
double last_update_time = glfwGetTime(), fall_update_time = glfwGetTime(), bullet_update_time = glfwGetTime();
float brickspeed = 0.05;
float genspeed = 1;
float xcd, ycd;

bool usemouse = false;

float xcen = 0, ycen = 0, zoom = 1;
int status = 0;

void createRectangle (string name, float x, float y, float width, float height, COLOR mycolor, string type);

int moveelem(Sprite& temp, float dx, float dy) 
{
    if(temp.x > 4 && dx > 0)
        return 0;
    if(temp.x < -4 && dx < 0)
        return 0;
    if(temp.y > 4 && dy > 0)
        return 0;
    if(temp.y < -4 && dy < 0)
        return 0;
    temp.x += dx;
    temp.y += dy;
    return 1;
}

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void show1(int digit)
{
  if(digit == 0)
  {
    scorebd["score12"].exist = 0;
    scorebd["score11"].exist = 1;
    scorebd["score13"].exist = 1;
    scorebd["score14"].exist = 1;
    scorebd["score15"].exist = 1;
    scorebd["score16"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 1)
  {
    scorebd["score11"].exist = 0;
    scorebd["score12"].exist = 0;
    scorebd["score13"].exist = 0;
    scorebd["score14"].exist = 0;
    scorebd["score16"].exist = 0;
    scorebd["score15"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 2)
  {
    scorebd["score14"].exist = 0;
    scorebd["score17"].exist = 0;
    scorebd["score11"].exist = 1;
    scorebd["score12"].exist = 1;
    scorebd["score13"].exist = 1;
    scorebd["score15"].exist = 1;
    scorebd["score16"].exist = 1;
  }
  else if(digit == 3)
  {
    scorebd["score14"].exist = 0;
    scorebd["score16"].exist = 0;
    scorebd["score11"].exist = 1;
    scorebd["score12"].exist = 1;
    scorebd["score13"].exist = 1;
    scorebd["score15"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 4)
  {
    scorebd["score11"].exist = 0;
    scorebd["score13"].exist = 0;
    scorebd["score16"].exist = 0;
    scorebd["score12"].exist = 1;
    scorebd["score14"].exist = 1;
    scorebd["score15"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 5)
  {
    scorebd["score15"].exist = 0;
    scorebd["score16"].exist = 0;
    scorebd["score11"].exist = 1;
    scorebd["score12"].exist = 1;
    scorebd["score13"].exist = 1;
    scorebd["score14"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 6)
  {
    scorebd["score15"].exist = 0;
    scorebd["score11"].exist = 1;
    scorebd["score12"].exist = 1;
    scorebd["score13"].exist = 1;
    scorebd["score14"].exist = 1;
    scorebd["score16"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 7)
  {
    scorebd["score12"].exist = 0;
    scorebd["score13"].exist = 0;
    scorebd["score14"].exist = 0;
    scorebd["score16"].exist = 0;
    scorebd["score11"].exist = 1;
    scorebd["score15"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 8)
  {
    scorebd["score11"].exist = 1;
    scorebd["score12"].exist = 1;
    scorebd["score13"].exist = 1;
    scorebd["score14"].exist = 1;
    scorebd["score15"].exist = 1;
    scorebd["score16"].exist = 1;
    scorebd["score17"].exist = 1;
  }
  else if(digit == 9)
  {
    scorebd["score16"].exist = 0;
    scorebd["score11"].exist = 1;
    scorebd["score12"].exist = 1;
    scorebd["score13"].exist = 1;
    scorebd["score14"].exist = 1;
    scorebd["score15"].exist = 1;
    scorebd["score17"].exist = 1;
  }
}

void show10(int digit)
{
  if(digit == 0)
  {
    scorebd["score22"].exist = 0;
    scorebd["score21"].exist = 1;
    scorebd["score23"].exist = 1;
    scorebd["score24"].exist = 1;
    scorebd["score25"].exist = 1;
    scorebd["score26"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 1)
  {
    scorebd["score21"].exist = 0;
    scorebd["score22"].exist = 0;
    scorebd["score23"].exist = 0;
    scorebd["score24"].exist = 0;
    scorebd["score26"].exist = 0;
    scorebd["score25"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 2)
  {
    scorebd["score24"].exist = 0;
    scorebd["score27"].exist = 0;
    scorebd["score21"].exist = 1;
    scorebd["score22"].exist = 1;
    scorebd["score23"].exist = 1;
    scorebd["score25"].exist = 1;
    scorebd["score26"].exist = 1;
  }
  else if(digit == 3)
  {
    scorebd["score24"].exist = 0;
    scorebd["score26"].exist = 0;
    scorebd["score21"].exist = 1;
    scorebd["score22"].exist = 1;
    scorebd["score23"].exist = 1;
    scorebd["score25"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 4)
  {
    scorebd["score21"].exist = 0;
    scorebd["score23"].exist = 0;
    scorebd["score26"].exist = 0;
    scorebd["score22"].exist = 1;
    scorebd["score24"].exist = 1;
    scorebd["score25"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 5)
  {
    scorebd["score25"].exist = 0;
    scorebd["score26"].exist = 0;
    scorebd["score21"].exist = 1;
    scorebd["score22"].exist = 1;
    scorebd["score23"].exist = 1;
    scorebd["score24"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 6)
  {
    scorebd["score25"].exist = 0;
    scorebd["score21"].exist = 1;
    scorebd["score22"].exist = 1;
    scorebd["score23"].exist = 1;
    scorebd["score24"].exist = 1;
    scorebd["score26"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 7)
  {
    scorebd["score22"].exist = 0;
    scorebd["score23"].exist = 0;
    scorebd["score24"].exist = 0;
    scorebd["score26"].exist = 0;
    scorebd["score21"].exist = 1;
    scorebd["score25"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 8)
  {
    scorebd["score21"].exist = 1;
    scorebd["score22"].exist = 1;
    scorebd["score23"].exist = 1;
    scorebd["score24"].exist = 1;
    scorebd["score25"].exist = 1;
    scorebd["score26"].exist = 1;
    scorebd["score27"].exist = 1;
  }
  else if(digit == 9)
  {
    scorebd["score26"].exist = 0;
    scorebd["score21"].exist = 1;
    scorebd["score22"].exist = 1;
    scorebd["score23"].exist = 1;
    scorebd["score24"].exist = 1;
    scorebd["score25"].exist = 1;
    scorebd["score27"].exist = 1;
  }
}

void show100(int digit)
{
  if(digit == 0)
    {
      scorebd["score32"].exist = 0;
      scorebd["score31"].exist = 1;
      scorebd["score33"].exist = 1;
      scorebd["score34"].exist = 1;
      scorebd["score35"].exist = 1;
      scorebd["score36"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 1)
    {
      scorebd["score31"].exist = 0;
      scorebd["score32"].exist = 0;
      scorebd["score33"].exist = 0;
      scorebd["score34"].exist = 0;
      scorebd["score36"].exist = 0;
      scorebd["score35"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 2)
    {
      scorebd["score34"].exist = 0;
      scorebd["score37"].exist = 0;
      scorebd["score31"].exist = 1;
      scorebd["score32"].exist = 1;
      scorebd["score33"].exist = 1;
      scorebd["score35"].exist = 1;
      scorebd["score36"].exist = 1;
    }
    else if(digit == 3)
    {
      scorebd["score34"].exist = 0;
      scorebd["score36"].exist = 0;
      scorebd["score31"].exist = 1;
      scorebd["score32"].exist = 1;
      scorebd["score33"].exist = 1;
      scorebd["score35"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 4)
    {
      scorebd["score31"].exist = 0;
      scorebd["score33"].exist = 0;
      scorebd["score36"].exist = 0;
      scorebd["score32"].exist = 1;
      scorebd["score34"].exist = 1;
      scorebd["score35"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 5)
    {
      scorebd["score35"].exist = 0;
      scorebd["score36"].exist = 0;
      scorebd["score31"].exist = 1;
      scorebd["score32"].exist = 1;
      scorebd["score33"].exist = 1;
      scorebd["score34"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 6)
    {
      scorebd["score35"].exist = 0;
      scorebd["score31"].exist = 1;
      scorebd["score32"].exist = 1;
      scorebd["score33"].exist = 1;
      scorebd["score34"].exist = 1;
      scorebd["score36"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 7)
    {
      scorebd["score32"].exist = 0;
      scorebd["score33"].exist = 0;
      scorebd["score34"].exist = 0;
      scorebd["score36"].exist = 0;
      scorebd["score31"].exist = 1;
      scorebd["score35"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 8)
    {
      scorebd["score31"].exist = 1;
      scorebd["score32"].exist = 1;
      scorebd["score33"].exist = 1;
      scorebd["score34"].exist = 1;
      scorebd["score35"].exist = 1;
      scorebd["score36"].exist = 1;
      scorebd["score37"].exist = 1;
    }
    else if(digit == 9)
    {
      scorebd["score36"].exist = 0;
      scorebd["score31"].exist = 1;
      scorebd["score32"].exist = 1;
      scorebd["score33"].exist = 1;
      scorebd["score34"].exist = 1;
      scorebd["score35"].exist = 1;
      scorebd["score37"].exist = 1;
    }
}

void showscore()
{
  int temp = score;
  
  if (temp <= 999) 
  {
    // cout << (temp/100)%10 << " " << (temp/10)%10 << " " << temp%10 << endl;
    show1(temp % 10);
    temp /= 10;
    show10(temp % 10);
    temp /= 10;
    show100(temp % 10);
  } 
  else 
  {
    show1(9);
    show10(9);
    show100(9);
  }
}

void shootBullet()
{
    float xcord = cannons["cannonhead"].x;
    float ycord = cannons["cannonhead"].y;
    stringstream ss;
    ss << "bullet";
    ss << numbullets;
    float myangle = cannons["cannonbody"].angle;
    float wid = cannons["cannonbody"].width;
    xcord = (xcord + 0.2) - (wid * cos(myangle *(M_PI/180)));
    ycord = ycord + (wid * sin(myangle *(M_PI/180)));

    createRectangle(ss.str(), xcord, ycord, 0.5, 0.05, lblue, "bullet");
    numbullets++;
}

void checkpan()
{
    float sf = pow(zoom, 4);
    float cf = zoom - 1;
    if(xcen - 4 / zoom < -4 - sf * cf)
        xcen = -4 + 4 / zoom - (sf * (zoom - 1)/4);
    else if(xcen + 4 / zoom > 4 + sf * cf)
        xcen = 4 - 4 / zoom + (sf * (zoom - 1)/4);
    if(ycen - 4 / zoom < -4 - sf * cf)
        ycen = -4 + 4 / zoom - (sf * (zoom - 1)/4);
    else if(ycen + 4 / zoom > 4 + sf * cf)
        ycen = 4 - 4 / zoom + (sf * (zoom - 1)/4);
}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_E:
                usemouse = !usemouse;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_LEFT:
                if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
                    moveelem(baskets["redbasket"], -0.2, 0);
                else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT))
                    moveelem(baskets["greenbasket"], -0.2, 0);
                else
                {
                    xcen -= 0.1;
                    checkpan();
                }
                break;
            case GLFW_KEY_RIGHT:
                if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
                    moveelem(baskets["redbasket"], 0.2, 0);
                else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT))
                    moveelem(baskets["greenbasket"], 0.2, 0);
                else
                {
                    xcen += 0.1;
                    checkpan();
                }
                break;
            case GLFW_KEY_UP:
                ycen += 0.1;
                checkpan();
                break;
            case GLFW_KEY_DOWN:
                ycen -= 0.1;
                checkpan();
                break;
            case GLFW_KEY_O:
                zoom /= 1.1;
                if(zoom <= 1)
                    zoom = 1;
                checkpan();
                break;
            case GLFW_KEY_P:
                zoom *= 1.1;
                if(zoom >= 4)
                    zoom = 4;
                checkpan();
                break;
            case GLFW_KEY_S:
                moveelem(cannons["cannonbody"], 0, 0.2);
                moveelem(cannons["cannonhead"], 0, 0.2);
                break;
            case GLFW_KEY_F:
                moveelem(cannons["cannonbody"], 0, -0.2);
                moveelem(cannons["cannonhead"], 0, -0.2);
                break;
            case GLFW_KEY_D:
                cannons["cannonhead"].angle -= 10.0f;
                break;
            case GLFW_KEY_A:
                cannons["cannonhead"].angle += 10.0f;
                break;
            case GLFW_KEY_N:
                if(genspeed >= 0.6)
                    genspeed -= 0.1;
                if(brickspeed <= 0.09)
                    brickspeed += 0.01;
                break;
            case GLFW_KEY_M:
                if(genspeed <= 1.4)
                    genspeed += 0.1;
                if(brickspeed > 0.01)
                    brickspeed -= 0.01;
                break;
            case GLFW_KEY_SPACE:
              current_time = glfwGetTime(); // Time in seconds
              if((current_time - bullet_update_time) >= 1.0) 
                { 
                    shootBullet();
                    bullet_update_time = current_time;
                }
              break;
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) 
            {
                current_time = glfwGetTime(); // Time in seconds
                if ((current_time - bullet_update_time) >= 1.0) 
                {
                shootBullet();
                bullet_update_time = current_time;
                }
            }
            break;
        case GLFW_MOUSE_BUTTON_LEFT:
            break;

        default:
            break;
    }
}

void mousescroll(GLFWwindow* window, double dx, double dy)
{
    if (dy == -1) 
    { 
        zoom /= 1.1; 
    }
    else if(dy == 1)
    {
        zoom *= 1.1; 
    }
    if (zoom <= 1) 
    {
        zoom = 1;
    }
    if (zoom >= 4) 
    {
        zoom = 4;
    }
    checkpan();
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
        int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
        glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	    GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
        // Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f,
        // 500.0f);
        float top = (ycen + 4) / zoom;
        float bottom = (ycen - 4) / zoom;
        float left = (xcen - 4) / zoom;
        float right = (xcen + 4) / zoom;
        Matrices.projection = glm::ortho(left, right, bottom, top, 0.1f, 500.0f);
}

// Creates the rectangle object used in this sample code
void createRectangle (string name, float x, float y, float width, float height, COLOR mycolor, string type)
{
  // create3DObject creates and returns a handle to a VAO that can be used later
  float w = width/2, h = height/2;
    GLfloat vertex_buffer_data [] = 
    {
        -w,-h,0, 
        -w,h,0, 
        w,h,0, 

        w,h,0, 
        w,-h,0, 
        -w,-h,0  
    };

  GLfloat color_buffer_data [] = {
        mycolor.r, mycolor.g, mycolor.b, // color 1
        mycolor.r, mycolor.g, mycolor.b, // color 2
        mycolor.r, mycolor.g, mycolor.b, // color 3

        mycolor.r, mycolor.g, mycolor.b, // color 4
        mycolor.r, mycolor.g, mycolor.b, // color 5
        mycolor.r, mycolor.g, mycolor.b // color 6
    };

  VAO *rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  Sprite elem = {};
  elem.color = mycolor;
  elem.angle = 0;
  elem.exist = 1;
  elem.name = name;
  elem.mirname = "";
  elem.object = rectangle;
  elem.x = x;
  elem.y = y;
  elem.height = height;
  elem.width = width;
  
  if(type == "basket")
    baskets[name] = elem;
  else if(type == "brick")
    bricks[name] = elem;
  else if(type == "cannon")
    cannons[name] = elem;
  else if(type == "mirror")
    mirrors[name] = elem;
  else if(type == "bg")
    bgs[name] = elem;
  else if(type == "scorebd")
    scorebd[name] = elem;
  else if(type == "bullet") 
  {
    elem.angle = cannons["cannonhead"].angle;
    bullets[name] = elem;
  }
}

float camera_rotation_angle = 90;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window)
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram(programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye(5 * cos(camera_rotation_angle * M_PI / 180.0f), 0,
                5 * sin(camera_rotation_angle * M_PI / 180.0f));
  // Target - Where is the camera looking at.  Don't change unless you are
  // sure!!
  glm::vec3 target(0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up(0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,1), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!

  float top = (ycen + 4) / zoom;
  float bottom = (ycen - 4) / zoom;
  float left = (xcen - 4) / zoom;
  float right = (xcen + 4) / zoom;
  Matrices.projection = glm::ortho(left, right, bottom, top, 0.1f, 500.0f);

  glm::mat4 VP = Matrices.projection * Matrices.view;

  int stater = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  if (stater == GLFW_PRESS) {
    {
      //cout << "hello" << endl;
      double xpos, ypos;
      int width, height;
      glfwGetWindowSize(window, &width, &height);
      glfwGetCursorPos(window, &xpos, &ypos);
      xcd = (xpos / width) * 8 - 4;
      //cout << "xcord " << xcd << endl;

      float gx = baskets["greenbasket"].x;
      float gy = baskets["greenbasket"].y;
      float rx = baskets["redbasket"].x;
      float ry = baskets["redbasket"].y;
      float wid = (baskets["redbasket"].width)/2;
      
      if (gx - wid < xcd && gx + wid > xcd && gy - wid < ycd && gy + wid > ycd && status!=2) {
        baskets["greenbasket"].x = xcd;
        status = 1;
      }
      else if (rx - wid < xcd && rx + wid > xcd && ry - wid < ycd && ry + wid > ycd && status!=1) {
        baskets["redbasket"].x = xcd;
        status = 2;
      }
      else
      {
          status = 0;
      }
    }
  }

  /* Render your scene */
  // draw baskets
  for (map<string, Sprite>::iterator it = baskets.begin(); it != baskets.end(); it++) 
  {
    string current = it->first; // The name of the current object
    if (baskets[current].exist == 0)
      continue;
    glm::mat4 MVP; // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate(glm::vec3(
        baskets[current].x, baskets[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle =
        glm::rotate((float)((0) * M_PI / 180.0f),
                    glm::vec3(0, 1, 0)); // rotate about vector (1,0,0)

    ObjectTransform = translateObject * rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(baskets[current].object);

    // glPopMatrix ();
  }
  // draw bricks
  for (map<string, Sprite>::iterator it = bricks.begin(); it != bricks.end(); it++) 
  {
    string current = it->first; // The name of the current object
    if (bricks[current].exist == 0)
      continue;
    glm::mat4 MVP; // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate(
        glm::vec3(bricks[current].x, bricks[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle =
        glm::rotate((float)((0) * M_PI / 180.0f),
                    glm::vec3(0, 1, 0)); // rotate about vector (1,0,0)

    ObjectTransform = translateObject * rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(bricks[current].object);

    // glPopMatrix (); 
  }
  double xpos, ypos;
  int width, height;
  float ycord = cannons["cannonhead"].y;
  glfwGetWindowSize(window, &width, &height);
  glfwGetCursorPos(window, &xpos, &ypos);
  //out << xpos << "-" << ypos << endl;
//   float num = (ypos - height/2 - (ycord * height)/8.0);
  //cout << num / xpos << endl;
//   float newangle = -1 * (atan( num / xpos) * 180)/(M_PI);
  //cout << "angle is" << newangle << endl;
  //cannons["cannonhead"].angle = newangle;	

  float xc = (xpos / width) * 8 - 4 - cannons["cannonhead"].x;
  float yc = (-1 * ypos / height) * 8 + 4 - cannons["cannonhead"].y;
  float newangle = atan(yc / xc) * 180 / M_PI;

  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  if (state == GLFW_PRESS) {
    {

      double xpos, ypos;
      int width, height;
      glfwGetWindowSize(window, &width, &height);
      glfwGetCursorPos(window, &xpos, &ypos);
      xcd = (xpos / width) * 8 - 4;
      ycd = (-1 * ypos / height) * 8 + 4;
      // cout << "ycord " << ycd << endl;

      float cx = cannons["cannonhead"].x;
      float cy = cannons["cannonhead"].y;
      if (cx - 1 < xcd && cx + 1 > xcd && cy - 1 < ycd && cy + 1 > ycd) {
        cannons["cannonhead"].y = ycd;
        cannons["cannonbody"].y = ycd;
      }
    }
  }

  if(usemouse == true)
    cannons["cannonhead"].angle = newangle;	

  //draw cannons
  for (map<string, Sprite>::iterator it = cannons.begin(); it != cannons.end(); it++) 
  {
    string current = it->first; // The name of the current object
    //cout << current << endl;
    if (cannons[current].exist == 0)
      continue;
    glm::mat4 MVP; // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate(glm::vec3(
        cannons[current].x, cannons[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((cannons[current].angle) * M_PI / 180.0f), glm::vec3(0, 0, 1)); 
        // rotate about vector (1,0,0)



    ObjectTransform = translateObject * rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(cannons[current].object);

    // glPopMatrix ();
  }
  // draw bullets
  for (map<string, Sprite>::iterator iter = bullets.begin(); iter != bullets.end(); iter++) 
  {
    string current = iter->first; // The name of the current object
    //cout << current << endl;
    if (bullets[current].exist == 0)
      continue;
    glm::mat4 MVP; // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate(glm::vec3(
        bullets[current].x, bullets[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((bullets[current].angle) * M_PI / 180.0f), glm::vec3(0, 0, 1)); 
    //     // rotate about vector (1,0,0)

    ObjectTransform = translateObject * rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // cout << "hey" << endl;

    //cout << "angle" << bullets[current].angle << endl;
    draw3DObject(bullets[current].object);

    // glPopMatrix ();
  }
  //printf("%d\n\n", bullets.size());
  //draw mirrors
  for (map<string, Sprite>::iterator it = mirrors.begin(); it != mirrors.end(); it++) 
  {
    string current = it->first; // The name of the current object
    //cout << current << endl;
    if (mirrors[current].exist == 0)
      continue;
    glm::mat4 MVP; // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate(glm::vec3(
        mirrors[current].x, mirrors[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((mirrors[current].angle) * M_PI / 180.0f), glm::vec3(0, 0, 1)); 
        // rotate about vector (1,0,0)

    ObjectTransform = translateObject * rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(mirrors[current].object);

    // glPopMatrix ();
  }
  for (map<string, Sprite>::iterator it = bgs.begin(); it != bgs.end(); it++) 
  {
    string current = it->first; // The name of the current object
    //cout << current << endl;
    if (bgs[current].exist == 0)
      continue;
    glm::mat4 MVP; // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate(glm::vec3(
        bgs[current].x, bgs[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((bgs[current].angle) * M_PI / 180.0f), glm::vec3(0, 0, 1)); 
        // rotate about vector (1,0,0)

    ObjectTransform = translateObject * rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(bgs[current].object);

    // glPopMatrix ();
  }

  for (map<string, Sprite>::iterator it = scorebd.begin(); it != scorebd.end(); it++) 
  {
    string current = it->first; // The name of the current object
    //cout << current << endl;
    if (scorebd[current].exist == 0)
    {
    //   cout << "not drawing" << current << endl;
      continue;
    }
    glm::mat4 MVP; // MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 ObjectTransform;
    glm::mat4 translateObject = glm::translate(glm::vec3(
        scorebd[current].x, scorebd[current].y, 0.0f)); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)((scorebd[current].angle) * M_PI / 180.0f), glm::vec3(0, 0, 1)); 
        // rotate about vector (1,0,0)

    ObjectTransform = translateObject * rotateTriangle;
    Matrices.model *= ObjectTransform;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(scorebd[current].object);

    // glPopMatrix ();
  }
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    // /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, mousescroll); // mouse scroll
    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
  /* Objects should be created before any other gl function and shaders */
  // Create the models
  // GL3 accepts only Triangles. Quads are not supported

  createRectangle("redbasket", -2, -3.5, 1.0, 0.5, red, "basket");
  createRectangle("greenbasket", 2, -3.5, 1.0, 0.5, green, "basket");
  createRectangle("cannonbody", -3.95, 0, 0.15, 0.7, black, "cannon");
  createRectangle("cannonhead", -4, 0, 0.8, 0.2, coolblue, "cannon");
  createRectangle("m1", 2.8, 1.6 + 1.7, 0.02, 0.8, silver, "mirror");
  createRectangle("m2", 1, 1.6, 0.02, 0.8, silver, "mirror");
  createRectangle("m3", 3, -1.0, 0.02, 0.8, silver, "mirror");
  createRectangle("m4", 1, -1.5, 0.02, 0.8, silver, "mirror");
  //createRectangle("m5", 0.7, 0 + 0.2, 0.02, 0.8, silver, "mirror");
  createRectangle("ground1", 0, -3.9, 10, 0.3, teal, "bg");
  mirrors["m1"].angle = 0.0f;
  mirrors["m2"].angle = 5.0f;
  mirrors["m3"].angle = 5.0f;
  mirrors["m4"].angle = -5.0f;
  //mirrors["m5"].angle = -50.0f;


  createRectangle("score11", 1, 3.5, 0.2, 0.05, black, "scorebd");
  createRectangle("score12", 1, 3.3, 0.2, 0.05, black, "scorebd");
  createRectangle("score13", 1, 3.1, 0.2, 0.05, black, "scorebd");
  createRectangle("score14", 0.9, 3.4, 0.05, 0.2, black, "scorebd");
  createRectangle("score15", 1.1, 3.4, 0.05, 0.2, black, "scorebd");
  createRectangle("score16", 0.9, 3.2, 0.05, 0.2, black, "scorebd");
  createRectangle("score17", 1.1, 3.2, 0.05, 0.2, black, "scorebd");

  createRectangle("score21", 0.7, 3.5, 0.2, 0.05, black, "scorebd");
  createRectangle("score22", 0.7, 3.3, 0.2, 0.05, black, "scorebd");
  createRectangle("score23", 0.7, 3.1, 0.2, 0.05, black, "scorebd");
  createRectangle("score24", 0.6, 3.4, 0.05, 0.2, black, "scorebd");
  createRectangle("score25", 0.8, 3.4, 0.05, 0.2, black, "scorebd");
  createRectangle("score26", 0.6, 3.2, 0.05, 0.2, black, "scorebd");
  createRectangle("score27", 0.8, 3.2, 0.05, 0.2, black, "scorebd");

  createRectangle("score31", 0.4, 3.5, 0.2, 0.05, black, "scorebd");
  createRectangle("score32", 0.4, 3.3, 0.2, 0.05, black, "scorebd");
  createRectangle("score33", 0.4, 3.1, 0.2, 0.05, black, "scorebd");
  createRectangle("score34", 0.3, 3.4, 0.05, 0.2, black, "scorebd");
  createRectangle("score35", 0.5, 3.4, 0.05, 0.2, black, "scorebd");
  createRectangle("score36", 0.3, 3.2, 0.05, 0.2, black, "scorebd");
  createRectangle("score37", 0.5, 3.2, 0.05, 0.2, black, "scorebd");

  // Create and compile our GLSL program from the shaders
  programID = LoadShaders("Sample_GL.vert", "Sample_GL.frag");
  // Get a handle for our "MVP" uniform
  Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

  reshapeWindow(window, width, height);

  // Background color of the scene
  glClearColor(1.0f, 1.0f, 1.0f, 0.0f); // R, G, B, A
  glClearDepth(1.0f);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
  cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
  cout << "VERSION: " << glGetString(GL_VERSION) << endl;
  cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

void bulletflow()
{
    for(map<string,Sprite>::iterator it=bullets.begin(); it!=bullets.end(); it++){
        string current = it->first; //The name of the current object
        if(bullets[current].exist==0)
            continue;
        float myangle = bullets[current].angle;
        // cout << myangle << endl;

        if(!moveelem(bullets[current], 0.2 * cos(myangle *(M_PI/180)), 0.2 * sin(myangle *(M_PI/180))))
            bullets[current].exist = 0;
    }
}

void checkcoll()
{
    for(map<string,Sprite>::iterator it=bricks.begin(); it!=bricks.end(); it++){
        string current = it->first; //The name of the current object
        if(bricks[current].exist==0)
            continue;
        if(bricks[current].y < -3.5)
        {
            //cout << "bye bye" << current << endl;
            //bricks.erase(it);
            // cout << &it << endl; //make an array with the needed to be deleted elems and delete after this loop
            bricks[current].exist = 0;
        }
    }
    for(map<string,Sprite>::iterator it=bullets.begin(); it!=bullets.end(); it++){
        string current = it->first; //The name of the current object
        if(bullets[current].exist==0)
            continue;
        if(bullets[current].x > 3.5)
        {
            //cout << "bye bye" << current << endl;
            //bricks.erase(it);
            // cout << &it << endl; //make an array with the needed to be deleted elems and delete after this loop
            bullets[current].exist = 0;
        }
    }
}

int checkdown(Sprite elem1, Sprite elem2)
{
    if( (elem1.y - (elem1.height)/2 <= elem2.y + (elem2.height)/2 ) && ((elem1.x + (elem1.width)/2) >= (elem2.x - (elem2.width)/2)) && ((elem1.x - (elem1.width)/2) <= (elem2.x + (elem2.width)/2)) )
    {
        //elem1.exist = 0;
        return 1;
    } 
    else
      return 0;
}

int checkbullcoll(Sprite bullet, Sprite block)
{
    float bulangle = bullet.angle;
    float x1 = bullet.x, y1 = bullet.y;
    float x2 = block.x, y2 = block.y;
    float w1 = (bullet.width)/2;
    float w2 = (block.width)/2;
    float h1 = (bullet.height)/2;
    float h2 = (block.height)/2;
    float dist = sqrt(w1*w1 + w2*w2 - 2*w1*w2*cos(bulangle *(M_PI/180)));
    float cendist = sqrt( (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2-y1) );
    if(cendist < dist + h1 + h2)
        return 1;
    else
        return 0;
}

int checkmirrcoll(Sprite bullet, Sprite mirror)
{
    float bulangle = bullet.angle;
    float mirangle = mirror.angle;
    float x1 = bullet.x, y1 = bullet.y;
    float x2 = mirror.x, y2 = mirror.y;
    float w1 = (bullet.width)/2;
    float w2 = (mirror.width)/2;
    float h1 = (bullet.height)/2;
    float h2 = (mirror.height)/2;
    float dist = sqrt(w1*w1 + w2*w2 - 2*w1*w2*cos((90.0f - bulangle + mirangle) *(M_PI/180)));
    float cendist = sqrt( (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2-y1) );
    if(cendist < dist + h1 + h2)
        return 1;
    else
        return 0;
}

//after sometime blocks disappear and shooting randomly destroys blocks
void bulletManage()
{
    for (map<string, Sprite>::iterator it1 = bullets.begin(); it1 != bullets.end(); it1++) 
    {
        string curbullet = it1->first;
        if(bullets[curbullet].exist == 0)
            continue;
 
        for (map<string, Sprite>::iterator it2 = bricks.begin(); it2 != bricks.end(); it2++)
        {
            string curbrick = it2->first;
            if(bricks[curbrick].exist == 0)
                continue;
 
            if(checkbullcoll(bullets[curbullet], bricks[curbrick]))
            {
                if(bricks[curbrick].color.r == (float)(30.0/255.0) && bricks[curbrick].color.g == (float)(30.0/255.0) && bricks[curbrick].color.b == (float)(21.0/255.0))
                {
                    score += 20;
                    bricks[curbrick].exist = 0;
                    bullets[curbullet].exist = 0;
                    cout << "Score is " << score << endl;
                }
                else
                {
                    score -= 5;
                    health -= 1;
                    cout << "Health is " << health << endl;
                    bricks[curbrick].exist = 0;
                    bullets[curbullet].exist = 0;
                    cout << "Score is " << score << endl;
                }
            }
        }

        for (map<string, Sprite>::iterator it3 = mirrors.begin(); it3 != mirrors.end(); it3++)
        {
            string curmirror = it3->first;
            if(checkmirrcoll(bullets[curbullet], mirrors[curmirror]))
            {
                //cout << "prev angle" << bullets[curbullet].angle << endl;
                if( (bullets[curbullet].mirname) != curmirror)
                {
                  float devangle = 2 * (  90 + mirrors[curmirror].angle - bullets[curbullet].angle);
                //   cout << "bullet: "<< bullets[curbullet].angle << endl;
                  bullets[curbullet].angle += devangle;
                  bullets[curbullet].mirname = curmirror;
                //   cout << "mirror: "<< mirrors[curmirror].angle << endl;
                //   cout << "angle of dev"<< devangle << endl;
                //   cout << "new bullet: "<< bullets[curbullet].angle << endl;
                  //cout << endl;
                }
                //cout << "new angle" << bullets[curbullet].angle << endl;
            }
        }
    }
}

void blockManage()
{
  for (map<string, Sprite>::iterator it = bricks.begin(); it != bricks.end(); it++) 
  {
    string current = it->first;
    float wid = baskets["redbasket"].width;
    float rx = baskets["redbasket"].x;
    float gx = baskets["greenbasket"].x;
    bool collect = true;

    if((rx + wid/2 > gx - wid/2) && (rx - wid/2 < gx + wid/2))
        collect = false;
    if((gx + wid/2 > rx - wid/2) && (gx - wid/2 < rx + wid/2))
        collect = false;

    if (bricks[current].exist == 1 && collect) 
    {
      if (checkdown(bricks[current], baskets["redbasket"])) 
      {
        if (bricks[current].color.r == 1.0) {
          score += 10;
          bricks[current].exist = 0;
          cout << "Score is " << score << endl;
        }
        else if (bricks[current].color.g == 1.0) {
          score -= 5;
          bricks[current].exist = 0;
          cout << "Score is " << score << endl;
        } 
        else {
          bricks[current].exist = 0;
        //   cout << "from red" << endl;
          health = 0;
          cout << "Health is " << health << endl;
        }
      }

      if (checkdown(bricks[current], baskets["greenbasket"])) 
      {
        if (bricks[current].color.g == 1.0) {
          score += 10;
          bricks[current].exist = 0;
          cout << "Score is " << score << endl;
        } 
        else if (bricks[current].color.r == 1.0) {
          score -= 5;
          bricks[current].exist = 0;
          cout << "Score is " << score << endl;
        }
        else {
          bricks[current].exist = 0;
        //   cout << "from green" << endl;
          health = 0;
          cout << "Health is " << health << endl;
        }
      }
    }
    else
      continue;

  }
}

void blockFall() 
{
  for (map<string, Sprite>::iterator it = bricks.begin(); it != bricks.end(); it++) 
  {
    string current = it->first;
    if(!moveelem(bricks[current], 0, -1*brickspeed))
        bricks[current].exist = 0;
  }

  current_time = glfwGetTime(); // Time in seconds
  if ((current_time - fall_update_time) >= (1.0 * genspeed)) 
  {
    srand(time(NULL));
    float randx = ((float)(rand() % 100) / 100) * 7;
    randx -= 3.5;

    srand(time(NULL));
    int rc = rand() % 3;
    COLOR randcolor;
    if (rc == 0)
      randcolor = red;
    else if (rc == 1)
      randcolor = green;
    else
      randcolor = black;

    stringstream ss;
    ss << numblocks;

    createRectangle(ss.str(), randx, 4, 0.2, 0.5, randcolor, "brick");
    numblocks++;
    fall_update_time = current_time;
  }
}

int main(int argc, char **argv) 
{
  int width = 900;
  int height = 600;

  GLFWwindow *window = initGLFW(width, height);

  initGL(window, width, height);

  cout << "Score is " << score << endl;
  cout << "Health is " << health << endl;

  mpg123_handle *mh;
  unsigned char *buffer;
  size_t buffer_size;
  size_t done;
  int err;

  int driver;
  ao_device *dev;

  ao_sample_format format;
  int channels, encoding;
  long rate;
  

  /* initializations */
  ao_initialize();
  driver = ao_default_driver_id();
  mpg123_init();
  mh = mpg123_new(NULL, &err);
  buffer_size = 3000;
  buffer = (unsigned char *)malloc(buffer_size * sizeof(unsigned char));

  /* open the file and get the decoding format */
  mpg123_open(mh, "music.mp3");
  mpg123_getformat(mh, &rate, &channels, &encoding);

  /* set the output format and open the output device */
  format.bits = mpg123_encsize(encoding) * BITS;
  format.rate = rate;
  format.channels = channels;
  format.byte_format = AO_FMT_NATIVE;
  format.matrix = 0;
  dev = ao_open_live(driver, &format, NULL);

  /* Draw in loop */
  while (!glfwWindowShouldClose(window)) {

    /* decode and play */
    if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
      ao_play(dev, (char *)buffer, done);
    else
      mpg123_seek(mh, 0, SEEK_SET);
    // OpenGL Draw commands
    if(health == 0)
    {
        cout << "GAME OVER"<< endl; 
        quit(window);
    }
    showscore();
    draw(window);
    checkcoll();
    blockManage();
    bulletManage();
    // draw3DObject draws the VAO given to it using current MVP matrix

    // Swap Frame Buffer in double buffering
    glfwSwapBuffers(window);

    // Poll for Keyboard and mouse events
    glfwPollEvents();

    // Control based on time (Time based transformation like 5 degrees
    // rotation every 0.5s)
    current_time = glfwGetTime(); // Time in seconds
    if ((current_time - last_update_time) >=
        1/24) { // atleast 0.5s elapsed since last frame
      // do something every 0.5 seconds ..
      blockFall();
      bulletflow();
      last_update_time = current_time;
    }
  }

  glfwTerminate();

  /* clean up */
  free(buffer);
  ao_close(dev);
  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
  ao_shutdown();
  
  return 0;
}
