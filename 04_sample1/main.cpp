//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "LiteMath.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>
#include <ctime>
static GLsizei WIDTH = 1024, HEIGHT = 768; //размеры окна

using namespace LiteMath;

float  cam_rot[2] = {-M_PI_4/3, -4 * M_PI_4};
int    mx = -3, my = 8;
float cx = 0, cy = 6, cz = -15;
float3 g_camPos(cx, cy, cz);
float3 direction;
float3 up(0, 1, 0);
float3 right;
float3 position(cx,cy,cz);
void windowResize(GLFWwindow* window, int width, int height)
{
  WIDTH  = width;
  HEIGHT = height;
}

void keyFun(GLFWwindow *window, int key, int scancode, int action, int mods) 
{
	right = float3(cos(cam_rot[0]) * cos(cam_rot[1]), 0, cos(cam_rot[0]) * sin(cam_rot[1]));
	direction = float3(cos(cam_rot[1] - 3.14f/2.0f), 0, sin(cam_rot[1] - 3.14f/2.0f));

	const float speed = 2;
  	float deltaTime = 0.1;
  	if (key == GLFW_KEY_D) {
    	position += right * deltaTime * speed;
  	}
  	if (key == GLFW_KEY_A) {
    	position -= right * deltaTime * speed;
  	}
  	if (key == GLFW_KEY_W) {
    	position += direction * deltaTime * speed;
  	}
  	if (key == GLFW_KEY_S) {
    	position -= direction * deltaTime * speed;
  	}
  	if (key == GLFW_KEY_R) {
    	position += up * deltaTime * speed;
  	}
  	if (key == GLFW_KEY_F) {
    	position -= up * deltaTime * speed;
  	}
}


int widthw;
int heighth;
unsigned char * get_tga_file(const char* t_file) 
{ 
  FILE *_file; 
  long size; 
  int color; 
  unsigned char header[18]; 
  _file = fopen(t_file, "rb"); 
  if(!_file) 
  { 
    std::cout << "cant open file " <<  t_file <<  std::endl;
    return 0; 
  } 
  fread(header,1,sizeof(header),_file); 
  if(header[2] != 2) { 
    std::cout << "!= 2 " <<  t_file <<  std::endl;
    return 0;} 
  widthw = header[13] * 256 + header[12]; 
  heighth = header[15] * 256 + header[14]; 
  color = header[16] / 8; 
  size = widthw * heighth * color; 
  unsigned char *image = new unsigned char[sizeof(unsigned char) * size]; 
  fread(image,sizeof(unsigned char),size,_file); 
  for(long i = 0; i < size; i += color) 
  { 
    unsigned char tmp = image[i]; 
    image[i] = image[i+2]; 
    image[i+2] = tmp; 
  } 
  fclose(_file);
  return image;
}

GLuint loadCubemap(std::vector<std::string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = get_tga_file(faces[i].c_str());
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, widthw, heighth, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            delete[] data;
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            delete[] data;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


static void mouseMove(GLFWwindow* window, double xpos, double ypos)
{ 
  xpos *= -0.75f;
  ypos *= 0.75f;
  //static int mx = xpos, my = ypos;

  //int x1 = int(xpos);
  //int y1 = int(ypos);
  //std::cout << "mx :" << mx << " my: " << my << std::endl;
  //std::cout << "xpos :" << xpos << " ypos: " << ypos << std::endl << std::endl;

  int x1 = xpos;
  int y1 = ypos;
  //std::cout << mx << 1 << std::endl;
  cam_rot[0] -= 0.025f*(y1 - my); //Изменение угола поворота
  if (cam_rot [0] > M_PI_2) {
    cam_rot[0] = M_PI_2;
  }
  if (cam_rot [0] < -M_PI_2) {
    cam_rot[0] = -M_PI_2;
  }
  cam_rot[1] -= 0.025f*(x1 - mx);

  mx = int(xpos);
  my = int(ypos);
}


int initGL()
{
	int res = 0;
	//грузим функции opengl через glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	std::cout << "Vendor: "   << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: "  << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL: "     << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	return 0;
}

int main(int argc, char** argv)
{


	if(!glfwInit())
    return -1;

	//запрашиваем контекст opengl версии 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE); 

    GLFWwindow*  window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL ray marching sample", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
    glfwSetCursorPosCallback (window, mouseMove);
    glfwSetWindowSizeCallback(window, windowResize);
    glfwSetKeyCallback(window, keyFun);

	glfwMakeContextCurrent(window); 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if(initGL() != 0) 
		return -1;
	
  //Reset any OpenGL errors which could be present for some reason
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR)
		gl_error = glGetError();

	//создание шейдерной программы из двух файлов с исходниками шейдеров
	//используется класс-обертка ShaderProgram
  std::vector<std::string> v = {"mp_sorbin/sorbin_ft.tga",
                                    "mp_sorbin/sorbin_bk.tga",
                                    "mp_sorbin/sorbin_dn.tga",
                                    "mp_sorbin/sorbin_up.tga",
                                    "mp_sorbin/sorbin_rt.tga",
                                    "mp_sorbin/sorbin_lf.tga" };
  GLuint cubetexture = loadCubemap(v); 
  std::unordered_map<GLenum, std::string> shaders;
  shaders[GL_VERTEX_SHADER]   = "vertex.glsl";
  shaders[GL_FRAGMENT_SHADER] = "fragment.glsl";
  ShaderProgram program(shaders); GL_CHECK_ERRORS;

    glfwSwapInterval(1); // force 60 frames per second
  
  //Создаем и загружаем геометрию поверхности
  //
    GLuint g_vertexBufferObject;
    GLuint g_vertexArrayObject;
    
 
    float quadPos[] =
    {
        -1.0f,  1.0f,	// v0 - top left corner
        -1.0f, -1.0f,	// v1 - bottom left corner
        1.0f,  1.0f,	// v2 - top right corner
        1.0f, -1.0f	  // v3 - bottom right corner
    };

    g_vertexBufferObject = 0;
    GLuint vertexLocation = 0; // simple layout, assume have only positions at location = 0

    glGenBuffers(1, &g_vertexBufferObject);                                                        GL_CHECK_ERRORS;
    glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);                                           GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), (GLfloat*)quadPos, GL_STATIC_DRAW);     GL_CHECK_ERRORS;

    glGenVertexArrays(1, &g_vertexArrayObject);                                                    GL_CHECK_ERRORS;
    glBindVertexArray(g_vertexArrayObject);                                                        GL_CHECK_ERRORS;

    glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);                                           GL_CHECK_ERRORS;
    glEnableVertexAttribArray(vertexLocation);                                                     GL_CHECK_ERRORS;
    glVertexAttribPointer(vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);                            GL_CHECK_ERRORS;

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubetexture);
    /*GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    */
    //glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    //цикл обработки сообщений и отрисовки сцены каждый кадр

    //unsigned int cubemapTexture = loadCubemap(v);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		srand(time(0));
		//очищаем экран каждый кадр
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);              // GL_CHECK_ERRORS;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //GL_CHECK_ERRORS;

    program.StartUseShader();                           //GL_CHECK_ERRORS;

    float4x4 camRotMatrix   = mul(rotate_Y_4x4(-cam_rot[1]), rotate_X_4x4(+cam_rot[0]));
    float4x4 camTransMatrix = translate4x4(position);
    float4x4 rayMatrix      = mul(camTransMatrix, camRotMatrix);
    program.SetUniform("g_rayMatrix", rayMatrix);  //изначально
    program.SetUniform("g_screenWidth" , WIDTH);
    program.SetUniform("g_screenHeight", HEIGHT);
    //program.SetUniform("time",(float) sin(rand()));

    // очистка и заполнение экрана цветом
    //
    glViewport  (0, 0, WIDTH, HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear     (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // draw call
    //
    glBindVertexArray(g_vertexArrayObject);// GL_CHECK_ERRORS;
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  //GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to VS invocations
    
    program.StopUseShader();

		glfwSwapBuffers(window); 
	}

	//очищаем vboи vao перед закрытием программы
  //
	glDeleteVertexArrays(1, &g_vertexArrayObject);
  glDeleteBuffers(1,      &g_vertexBufferObject);

	glfwTerminate();
	return 0;
}
