#include <windows.h>

#include <GL/glew.h>

#include <GL/freeglut.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include "imgui_impl_glut.h"
#include "VideoMux.h"
#include "InitShader.h"
#include "LoadMesh.h"
#include "LoadTexture.h"

static const std::string vertex_shader("fbo_vs.glsl");
static const std::string fragment_shader("fbo_fs.glsl");

GLuint shader_program = -1;
GLuint texture_id = -1; //Texture map for fish
GLuint pick_texture_id = -1;

GLuint quad_vao = -1;
GLuint quad_vbo = -1;

GLuint fbo_id = -1;       // Framebuffer object,
GLuint rbo_id = -1;       // and Renderbuffer (for depth buffering)
GLuint fbo_texture = -1; // Texture rendered into.
GLuint fbo2_texture = -1; 

//int w = 1280;
//int h = 720;
static const int InstanceNum = 12;
static const std::string mesh_name = "Amago0.obj";
static const std::string texture_name = "AmagoT.bmp";
static const std::string pick_texture_name = "blue.jpg";

MeshData mesh_data;
float time_sec = 0.0f;

int pickedID = -1;
bool check_framebuffer_status();
bool recording = false;
bool edgeDetection = false;


void reload_shader()
{
	GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

	if (new_shader == -1) // loading failed
	{
		glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

		if (shader_program != -1)
		{
			glDeleteProgram(shader_program);
		}
		shader_program = new_shader;

		if (mesh_data.mVao != -1)
		{
			BufferIndexedVerts(mesh_data);
		}
	}
}
void draw_gui()
{
	ImGui_ImplGlut_NewFrame();
	ImGui::Image((void*)fbo2_texture, ImVec2(60, 60));

	const int filename_len = 256;
	static char video_filename[filename_len] = "capture.mp4";

	ImGui::Checkbox("Enable Edge Detection", &edgeDetection);

	ImGui::InputText("Video filename", video_filename, filename_len);
	//ImGui::SameLine();
	const int w = glutGet(GLUT_SCREEN_WIDTH);
	const int h = glutGet(GLUT_SCREEN_HEIGHT);
	if (recording == false)
	{
		if (ImGui::Button("Start Recording"))
		{
			recording = true;
			start_encoding(video_filename, w, h); //Uses ffmpeg
		}

	}
	else
	{
		if (ImGui::Button("Stop Recording"))
		{
			recording = false;
			finish_encoding(); //Uses ffmpeg
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload Shader")) {
		printf("reload shader\n");
		reload_shader();
	}
	ImGui::Render();
}
void draw_pass_1()
{
   const int pass = 1;

   //glm::mat4 M = glm::rotate(10.0f*time_sec, glm::vec3(0.0f, 1.0f, 0.0f))*glm::scale(glm::vec3(mesh_data.mScaleFactor))*glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
   glm::mat4 M = glm::scale(glm::vec3(mesh_data.mScaleFactor))*glm::scale(glm::vec3(0.2f, 0.2f, 0.2f));
   glm::mat4 V = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.3f, 0.2f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 P = glm::perspective(50.0f, 1.0f, 0.1f, 10.0f);

   int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }


   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texture_id);
   glActiveTexture(GL_TEXTURE0 + 1);
   glBindTexture(GL_TEXTURE_2D, pick_texture_id);

   int PVM_loc = glGetUniformLocation(shader_program, "PVM");
   if(PVM_loc != -1)
   {
      glm::mat4 PVM = P*V*M;
      glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
   }
   
   int tex_loc = glGetUniformLocation(shader_program, "texture");
   if(tex_loc != -1)
   {
      glUniform1i(tex_loc, 0); // we bound our texture to texture unit 0
   }
   
   int pick_tex_loc = glGetUniformLocation(shader_program, "pick_texture");
   if (pick_tex_loc != -1)
   {
	   glUniform1i(pick_tex_loc, 1); // we bound our texture to texture unit 0
   }

   int pickedID_loc = 2;
   glUniform1i(pickedID_loc, pickedID);

   glBindVertexArray(mesh_data.mVao);
   glDrawElementsInstanced(GL_TRIANGLES, mesh_data.mNumIndices, GL_UNSIGNED_INT, 0, InstanceNum);

}

void draw_pass_2()
{
	//pass 2 is edge detection
   const int pass = 2;
   int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, fbo_texture);

   int tex_loc = glGetUniformLocation(shader_program, "texture");
   if(tex_loc != -1)
   {
      glUniform1i(tex_loc, 0); // we bound our texture to texture unit 0
   }

   glBindVertexArray(quad_vao);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{

   glUseProgram(shader_program);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id); // Render to FBO.
   
   //Out variable in frag shader will be written to the texture attached to GL_COLOR_ATTACHMENT0 or 1.
   GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
   glDrawBuffers(2, drawBuffers);


   //Make the viewport match the FBO texture size.
   int tex_w, tex_h;
   glBindTexture(GL_TEXTURE_2D, fbo_texture);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_w);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_h);
   glViewport(0, 0, tex_w, tex_h);

   //Clear the FBO.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Lab assignment: don't forget to also clear depth
   draw_pass_1();

   glBindFramebuffer(GL_FRAMEBUFFER, 0); // Do not render the next pass to FBO.
   glDrawBuffer(GL_BACK); // Render to back buffer.
   
   //const int w = glutGet(GLUT_WINDOW_WIDTH);
   //const int h = glutGet(GLUT_WINDOW_HEIGHT);
   const int w = glutGet(GLUT_SCREEN_WIDTH);
   const int h = glutGet(GLUT_SCREEN_HEIGHT);

   glViewport(0, 0, w, h); //Render to the full viewport
   glClearColor(1.0, 1.0, 1.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear the back buffer

   if (edgeDetection == true) {
	   draw_pass_2();
   }
   else {
	   draw_pass_1();
   }

   draw_gui();
   if (recording == true)
   {
	   glFinish();

	   glReadBuffer(GL_BACK);
	   read_frame_to_encode(&rgb, &pixels, w, h);
	   encode_frame(rgb);
   }


   glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   time_sec = 0.001f*time_ms;

   
   GLint timeLoc;
   timeLoc = glGetUniformLocation(shader_program, "time");
   glUniform1f(timeLoc, time_sec);  
   

}



// glut keyboard callback function.
// This function gets called when an ASCII key is pressed
void keyboard(unsigned char key, int x, int y)
{
   std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;

   switch(key)
   {
      case 'r':
      case 'R':
         reload_shader();     
      break;
   }
}

void printGlInfo()
{
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
}

void resizeWindow(int width, int height) {

}
void initOpenGl()
{
   glewInit();

   glEnable(GL_DEPTH_TEST);

   reload_shader();

   //mesh and texture for pass 1
   mesh_data = LoadMesh(mesh_name);
   texture_id = LoadTexture(texture_name.c_str());
   pick_texture_id = LoadTexture(pick_texture_name.c_str());

   //mesh for pass 2 (full screen quadrilateral)
   glGenVertexArrays(1, &quad_vao);
   glBindVertexArray(quad_vao);

   float vertices[] = {1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f};

   //create vertex buffers for vertex coords
   glGenBuffers(1, &quad_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   int pos_loc = glGetAttribLocation(shader_program, "pos_attrib");
   if(pos_loc >= 0)
   {
      glEnableVertexAttribArray(pos_loc);
	  glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);
   }

   //const int w = glutGet(GLUT_WINDOW_WIDTH);
   //const int h = glutGet(GLUT_WINDOW_HEIGHT);
   const int w = glutGet(GLUT_SCREEN_WIDTH);
   const int h = glutGet(GLUT_SCREEN_HEIGHT);

   //Create texture to render pass 1 into.
   glGenTextures(1, &fbo_texture);
   glBindTexture(GL_TEXTURE_2D, fbo_texture);
   //Lab assignment: make the texture width and height be the window width and height.
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glBindTexture(GL_TEXTURE_2D, 0);   

   
   //Create texture to render picked mesh
   glGenTextures(1, &fbo2_texture);
   glBindTexture(GL_TEXTURE_2D, fbo2_texture);
   //Lab assignment: make the texture width and height be the window width and height.
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);
   

   //Lab assignment: Create renderbuffer for depth.
   glGenRenderbuffers(1, &rbo_id);
   glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h); // same size as the window
   
   //Create the framebuffer object
   glGenFramebuffers(1, &fbo_id);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fbo2_texture, 0);

   //Lab assignment: attach depth renderbuffer to FBO
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_id);

   check_framebuffer_status();

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void motion(int x, int y)
{
	ImGui_ImplGlut_MouseMotionCallback(x, y);
}
void mouse(int button, int state, int x, int y)
{
	ImGui_ImplGlut_MouseButtonCallback(button, state);

	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
	{
		GLubyte buffer[4];//pixel data
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		
		const int h = glutGet(GLUT_WINDOW_HEIGHT);
		glReadPixels(x, h - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		pickedID = (int)buffer[0];
		if(pickedID < InstanceNum && pickedID >=0)
			std::cout << "\npicked ID: " << pickedID;
	}

}


int main (int argc, char **argv)
{
   //Configure initial window state
   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (1280, 720);
   int win = glutCreateWindow ("HW2 FBO solution");

   printGlInfo();

   //Register callback functions with glut. 
   glutDisplayFunc(display); 
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutPassiveMotionFunc(motion);

   glutReshapeFunc(resizeWindow);

   initOpenGl();
   ImGui_ImplGlut_Init(); // initialize the imgui system

   //Enter the glut event loop.
   glutMainLoop();
   glutDestroyWindow(win);
   return 0;		
}

bool check_framebuffer_status() 
{
    GLenum status;
    status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			printf("Framebuffer incomplete, incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED:
			printf("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			printf("Framebuffer incomplete, missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			printf("Framebuffer incomplete, missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			printf("Framebuffer incomplete, missing read buffer\n");
            return false;
    }
	return false;
}


