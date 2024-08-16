#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<iostream>

void framebuffer_size_callback( GLFWwindow* window, int width, int height );
void input( GLFWwindow* window );

float vertices[] = {
	0.5f, 0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	-0.5f, 0.5f, 0.0f
};
unsigned int indices[] = {
	0, 1, 3,
	1, 2, 3
};
unsigned int VBO;
unsigned int VAO;
unsigned int EBO;
const char *vertexShaderSource = "#version 330 core\n"
	"layout( location = 0 ) in vec3 aPos;\n"
	"void main( void ) {\n"
	"gl_Position = vec4( aPos.x, aPos.y, aPos.z, 1.0 );\n"
	"}\0";
const char *fragmentShaderSource = "#version 330 core\n"
	"out vec4 FragColor;\n"
	"void main( void ) {\n"
	"FragColor = vec4( 1.0f, 0.5, 0.2f, 1.0f );\n"
	"}\0";

bool polymode = false;
bool alreadypressed = false;

int main( void ) {
	glfwInit();
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

	GLFWwindow* window = glfwCreateWindow( 800, 600, "OPENGL", NULL, NULL );
	if( window == NULL ) {
		std::cout<<"Failed to create window"<<std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent( window );

	if( !gladLoadGLLoader( ( GLADloadproc )glfwGetProcAddress ) ) {
		std::cout<<"Failed to initialize GLAD"<<std::endl;
		return -1;
	}

	glViewport( 0, 0, 800, 600 );
	glfwSetFramebufferSizeCallback( window, framebuffer_size_callback );

	unsigned int vertexShader;
	vertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vertexShader, 1, &vertexShaderSource, NULL );
	glCompileShader( vertexShader );
	int success;
	char infoLog[ 512 ];
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &success );
	if( !success ) {
		glGetShaderInfoLog( vertexShader, 512, NULL, infoLog );
		std::cout<<"ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"<<infoLog<<std::endl;
	}

	unsigned int fragmentShader;
	fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fragmentShader, 1, &fragmentShaderSource, NULL );
	glCompileShader( fragmentShader );
	glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &success );
	if( !success ) {
		glGetShaderInfoLog( vertexShader, 512, NULL, infoLog );
		std::cout<<"ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"<<infoLog<<std::endl;
	}

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();
	glAttachShader( shaderProgram, vertexShader );
	glAttachShader( shaderProgram, fragmentShader );
	glLinkProgram( shaderProgram );
	glGetProgramiv( shaderProgram, GL_LINK_STATUS, &success );
	if( !success ) {
		glGetProgramInfoLog( shaderProgram, 512, NULL, infoLog );
		std::cout<<"ERROR: "<<infoLog<<std::endl;
	}
	glUseProgram( shaderProgram );
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );

	glGenBuffers( 1, &VBO );
	glGenVertexArrays( 1, &VAO );
	glGenBuffers( 1, &EBO );
	glBindBuffer( GL_ARRAY_BUFFER, VBO );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );
	glBindVertexArray( VAO );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );
	glEnableVertexAttribArray( 0 );

	while( !glfwWindowShouldClose( window ) ) {
		input( window );

		glClearColor( 0.2f, 0.3f, 0.3f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT );

		//glBindBuffer( GL_ARRAY_BUFFER, VBO );
		//glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

		glUseProgram( shaderProgram );
		glBindVertexArray( VAO );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
		glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

		glfwSwapBuffers( window );
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
void input( GLFWwindow *window ) {
	if( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) {
		glfwSetWindowShouldClose( window, true );
	}
	if( glfwGetKey( window, GLFW_KEY_F1 ) == GLFW_PRESS ) {
		if( polymode ) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			polymode = !polymode;
		} else {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			polymode = !polymode;
		}
	}
}
void framebuffer_size_callback( GLFWwindow* window, int width, int height ) {
	glViewport( 0, 0, width, height );
}