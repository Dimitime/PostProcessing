#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include <math.h>
#include <vector>
#include <iostream>

#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/glm.hpp>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

//Shaders
GLuint phongshader,ppshader,kernelshader,bloomshader,bloomblurh,bloomblurv;
//OGL Buffer objects
GLuint vao, vbo[2], qvbo, fbo;
//FBO texture/depth
GLuint tex[2], depthbuffer;
GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

//control for shaders
int shader = 0;

std::vector<glm::vec3> vertices;
std::vector<glm::vec3> normals;
std::vector<GLushort> faces;

glm::mat4 Projection = glm::ortho(-0.35,0.35,-0.35,0.35);
glm::mat4 View = glm::lookAt(glm::vec3(0.0f,0.0f,1.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
glm::mat4 Model = glm::mat4(1.f);

GLfloat quad[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f
};

void phong() {
	glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	
	//Vertex normals
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);	
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	glEnableVertexAttribArray(1);

	GLuint m,v,p;
	//Draw the models

	//For the bloom shader we only render the diffuse and ambient
	if (shader == 4) {
		glUseProgram(bloomshader);
		m = glGetUniformLocation(bloomshader, "model");
		v = glGetUniformLocation(bloomshader, "view");
		p = glGetUniformLocation(bloomshader, "projection");
	}
	else {
		glUseProgram(phongshader);
		m = glGetUniformLocation(phongshader, "model");
		v = glGetUniformLocation(phongshader, "view");
		p = glGetUniformLocation(phongshader, "projection");
	}

	glUniformMatrix4fv(m, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(v, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(p, 1, GL_FALSE, &Projection[0][0]);

	//Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}

void passthrough() {
	glUseProgram(kernelshader);
	
	GLuint texID = glGetUniformLocation(kernelshader, "texture");
	GLuint kID = glGetUniformLocation(kernelshader, "kernel");
	GLuint rID = glGetUniformLocation(kernelshader, "radius");
	glm::mat3 kernel = glm::mat3(0, 0, 0,
								 0, 1, 0,
								 0, 0, 0);
	float radius = 1.0f / 300.0f;
	
	glUniformMatrix3fv(kID, 1, GL_FALSE, &kernel[0][0]);
	glUniform1f(rID, radius);
	glUniform1i(texID, 0);
}

void sharpen_kernel() {
	glUseProgram(kernelshader);
	
	GLuint texID = glGetUniformLocation(kernelshader, "texture");
	GLuint kID = glGetUniformLocation(kernelshader, "kernel");
	GLuint rID = glGetUniformLocation(kernelshader, "radius");
	glm::mat3 kernel = glm::mat3(-1, -1, -1,
								 -1,  9, -1,
								 -1, -1, -1);
	float radius = 1.0f / 300.0f;
	
	glUniformMatrix3fv(kID, 1, GL_FALSE, &kernel[0][0]);
	glUniform1f(rID, radius);
	glUniform1i(texID, 0);
}

void gauss_kernel() {
	//float *gauss_matrix = (float *) malloc (sizeof (float) * (2*radius+1) * (2*radius+1));
	glm::mat3 kernel = glm::mat3(1.0);
    float g_coeff = 1/(2*(float)M_PI*pow((1.0f/3.0f),2));
    float tsigmas = 2*pow((1.0f/3.0f),2);
    float mid = 3.0f/2.0f;
    float g_sum = 0.0;

    //precompute the gaussian matrix
    for (int j=0; j<3; j++) {
       for (int i=0; i<3; i++) {
          float gauss_value = g_coeff*exp( -( (pow(i-mid,2)+pow(j-mid,2))/(tsigmas) ) );
          kernel[i][j] = gauss_value;
          g_sum += gauss_value;
       }
    }

    //normalize the kernel
    for (int j=0; j<3; j++) {
       for (int i=0; i<3; i++) {
          kernel[i][j] = kernel[i][j]/g_sum;
       }
    }
	
	float radius = 2.0f/500.0f;

	glUseProgram(kernelshader);
	
	GLuint texID = glGetUniformLocation(kernelshader, "texture");
	GLuint kID = glGetUniformLocation(kernelshader, "kernel");
	GLuint rID = glGetUniformLocation(kernelshader, "radius");
	
	glUniformMatrix3fv(kID, 1, GL_FALSE, &kernel[0][0]);
	glUniform1f(rID, radius);
	glUniform1i(texID, 0);
}

void wave() {
	GLfloat t = glutGet(GLUT_ELAPSED_TIME)/1000.0f * 2*(float)M_PI;

	glUseProgram(ppshader);
	
	GLuint texID = glGetUniformLocation(ppshader, "texture");
	GLuint tID = glGetUniformLocation(ppshader, "t");
	glUniform1i(texID, 0);
	glUniform1f(tID, t);
}

//we blur in 2 passes, this is the horizontal pass
void bloom_blur_h() {
	
	float radius = 2.5f/WINDOW_WIDTH;

	glUseProgram(bloomblurh);
	
	GLuint texID = glGetUniformLocation(bloomblurh, "texture");
	GLuint rID = glGetUniformLocation(bloomblurh, "radius");
	glUniform1f(rID, radius);
	glUniform1i(texID, 0);
}

//we blur in 2 passes, this is the vertical pass
void bloom_blur_v() {
	
	float radius = 2.5f/WINDOW_HEIGHT;

	glUseProgram(bloomblurv);
	
	GLuint texID = glGetUniformLocation(bloomblurv, "texture");
	GLuint rID = glGetUniformLocation(bloomblurv, "radius");
	glUniform1f(rID, radius);
	glUniform1i(texID, 0);
}

void choose_post_shader() {
	switch (shader) {
		case 0:
			passthrough();
			break;
		case 1:
			wave();
			break;
		case 2:
			sharpen_kernel();
			break;
		case 3:
			gauss_kernel();
			break;
		case 4:
			break;
		default:
			passthrough();
			break;
	}
}

static void postprocess() {
	//Render the quad
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	choose_post_shader();

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, qvbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	glutSwapBuffers();
}

//Bloom consists of blurring the specular map
void bloom() {
	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex[0]);

/*	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
*/

	bloom_blur_h();
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, qvbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0);
	
	//Render the quad
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	
	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex[1]);


	bloom_blur_v();

	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, qvbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0);
}

static void disp(void) {
	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);

	phong();

	if (shader == 4)
		bloom();
	else
		postprocess();

	//unbind everything
	glUseProgram(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glutSwapBuffers();
}

static void idle() {
	int timems = glutGet(GLUT_ELAPSED_TIME);

	if (timems % 100 == 0) {
        glutPostRedisplay();
	}
}

static void keyboard(unsigned char key, int x, int y) {
    switch (key) {
		case '0':
			shader = 0;
			break;
		case '1':
			shader = 1;
			break;
		case '2':
			shader = 2;
			break;
		case '3':
			shader = 3;
			break;
		case '4':
			shader = 4;
			break;
		case 27:
			exit(0);
    }
}

glm::vec3 center(std::vector<glm::vec3> pos) {
	float maxx=pos[0].x;
	float minx=pos[0].x;
	float maxy=pos[0].y;
	float miny=pos[0].y;
	float maxz=pos[0].z;
	float minz=pos[0].z;

	for (size_t i=0; i<pos.size(); i++) {
		if (pos[i].x > maxx)
			maxx = pos[i].x;
		if (pos[i].x < minx)
			minx = pos[i].x;
		if (pos[i].y > maxy)
			maxy = pos[i].y;
		if (pos[i].y < miny)
			miny = pos[i].y;
		if (pos[i].z > maxz)
			maxz = pos[i].z;
		if (pos[i].z < minz)
			minz = pos[i].z;
	}

	glm::vec3 ret(0.0f);
	ret.x = (maxx + minx) / 2.0f;
	ret.y = (maxy + miny) / 2.0f;
	ret.z = (maxz + minz) / 2.0f;

	std::cout << "Center: " << ret.x << "," << ret.y << "," << ret.z << std::endl;

	return ret;
}

void loadObj(const char* filename, std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<GLushort> &faces) {
	std::ifstream in(filename, std::ios::in);
	if (!in)
		std::cerr << "Cannot open " << filename << std::endl;
 
	//Temporary containers
	std::vector<glm::vec3> tv;
	std::vector<glm::vec3> tn;

	//an array of indices
	std::vector<int> vi;
	std::vector<int> ni;

	std::string line;
	while (std::getline(in, line)) {
		//We have vertex data
		if (line.substr(0,2) == "v ") {
			std::istringstream s(line.substr(2));
			glm::vec3 v; s >> v.x; s >> v.y; s >> v.z;
			tv.push_back(v);
		}
		//We have vertex normals
		else if (line.substr(0,2) == "vn") {
			std::istringstream s(line.substr(2));
			glm::vec3 n; s >> n.x; s >> n.y; s >> n.z;
			tn.push_back(n);
		}
		//Face info. A little hacky, I'm assuming no texture coordinates.
		else if (line.substr(0,2) == "f ") {
			std::istringstream s(line.substr(2));
			//std::cout << s.str() << std::endl;

			GLushort v1,v2,v3,n1,n2,n3;
			char s1;
			//format is v1//n1 v2//n2 v3//n3
			s>>v1; s>>s1; s>>s1; s>>n1; 
			s>>v2; s>>s1; s>>s1; s>>n2;
			s>>v3; s>>s1; s>>s1; s>>n3;
			
			faces.push_back(v1);
			vi.push_back(v1); vi.push_back(v2); vi.push_back(v3);
			ni.push_back(n1); ni.push_back(n2); ni.push_back(n3);
		}
	}

	//Now we rearrange the vertex and normal arrays based on the indices found in the faces
	for (size_t i=0; i<vi.size(); i++) {
		 glm::vec3 v = tv[vi[i]-1];
		 vertices.push_back(v);
		 glm::vec3 n = tn[ni[i]-1];
		 normals.push_back(n);
	}
}

//Loading shelders
GLuint loadShaders(const char * vertex_file_path, const char * fragment_file_path) {
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
 
    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
    {
        std::string Line = "";
        while(std::getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }
 
    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(std::getline(FragmentShaderStream, Line))
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
    std::vector<char> ProgramErrorMessage( glm::max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);
 
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

//initialize everything
static void init(void) {
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	//Create the VAO	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Load the shaders
	phongshader = loadShaders("Shaders/phong.vs", "Shaders/phong.fs");
	ppshader = loadShaders("Shaders/pp.vs", "Shaders/pp.fs");
	kernelshader = loadShaders("Shaders/pp.vs", "Shaders/ks.fs");
	bloomshader = loadShaders("Shaders/phong.vs", "Shaders/bloom.fs");
	bloomblurh = loadShaders("Shaders/pp.vs", "Shaders/bloom_blur_h.fs");
	bloomblurv = loadShaders("Shaders/pp.vs", "Shaders/bloom_blur_v.fs");

	//Load the models
	loadObj("Meshes/me100k.obj",vertices,normals,faces);
	std::cout << "mesh loaded" << std::endl;
	
	Model = glm::scale(Model, glm::vec3(1.5f,1.5f,1.5f));
	Model = glm::translate(Model, glm::vec3(0.0f,0.05f,0.0f));

	Model = glm::translate(Model, center(vertices));
	Model = glm::rotate(Model, 1.45f, glm::vec3(-1.0f, 0.0f, 0.0f));
	Model = glm::rotate(Model, 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
	Model = glm::translate(Model, -center(vertices));

	//Create vertex buffer object
	glGenBuffers(2, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	
	//Initialize the vbo to the initial positions
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);

	//Create the FBO we will render to
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	//The texture we're going to render to
	glGenTextures(2, tex);
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, tex[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// The depth buffer
	glGenRenderbuffers(1, &depthbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex[0], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex[1], 0);
	glDrawBuffers(2, drawBuffers);
	// Always check that our framebuffer is ok
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		exit(0);

	//Generate the quad buffer
	glGenBuffers(1, &qvbo);
	glBindBuffer(GL_ARRAY_BUFFER, qvbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

	std::cout <<"Buffer setup complete" << std::endl;
}

int main(int argc, char** argv) {
	// init glut:
	glutInit (&argc, argv);
    //glutInitContextVersion(3,3);
    //glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(0,0);
	glutCreateWindow("SPH");	
	//glutFullScreen();
    //glutGameModeString("1280x720:16@60"); glutEnterGameMode();
	printf("OpenGL Version:%s\n",glGetString(GL_VERSION));
	printf("GLSL Version  :%s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));

	glutDisplayFunc(&disp);
    glutIdleFunc(&idle);
    glutKeyboardFunc(&keyboard);

	glewExperimental=GL_TRUE;
	GLenum err=glewInit();
	if(err!=GLEW_OK)
		printf("glewInit failed, aborting.\n");
    if (!glewIsSupported("GL_VERSION_3_3 ")) {
        fprintf(stderr, "ERROR: Support for necessary OpenGL extensions missing.");
        fflush(stderr);
		exit(0);
    }

	init();

	// enter tha main loop and process events:
	glutMainLoop();
	return 0;
}