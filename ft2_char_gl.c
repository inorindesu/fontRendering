/* 
 * Draw a character with OpenGL
 * 
 * Copyright (C) 2013  Inori Sakura <inorindesu@gmail.com>
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h>
#include <GLFW/glfw3.h>

typedef unsigned int uint;
typedef unsigned char uchar;

/* utf32 representation of some characters */
#define TA      0x062a /*Arabic ta*/
#define THA     0x062b /*Arabic tha*/
#define CANCER  0x264b /*Cancer*/

#define BACK_R  (249)
#define BACK_G  (202)
#define BACK_B  (222)
#define FORE_R  (44)
#define FORE_G  (0)
#define FORE_B  (89)

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480
#define FONTPATH ("/usr/share/fonts/dejavu/DejaVuSans.ttf")
#define CHARCOUNT (3)
uint chars[CHARCOUNT] = {TA, THA, CANCER};
GLuint charTextures[CHARCOUNT];
int curTextureIdx = 0;

/* fragment shader */
const char* vertexShader = "#version 120\n"
"attribute vec3 vertexPosition;\n"
"attribute vec2 vertexUV;\n"
"varying vec2 UV;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(vertexPosition, 1.0);\n"
"  UV = vertexUV;\n"
"}\n";

const char* fragShader = "#version 120\n"
"varying vec2 UV;\n"
"uniform vec3 backColor;\n"
"uniform vec3 foreColor;\n"
"uniform sampler2D myTexture;\n"
"void main()\n"
"{\n"
"  float textureAlpha = texture2D(myTexture, UV).r;\n"
"  vec3 finalColor = backColor * (1 - textureAlpha) + foreColor * textureAlpha;\n"
"  gl_FragColor = vec4(finalColor, 1.0f);\n"
"}\n";

float vertices[] = 
  {
    -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f
  };

float uv[] =
  {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f
  };

/*Counter-Clockwise*/
int vboIdx[] =
  {
    0, 3, 1, 0, 2 ,3
  };

int max(int a, int b)
{
  return a > b ? a : b;
}

int min(int a, int b)
{
  return a < b? a : b;
}

int get_appropriate_power_of_two(int v)
{
  int i;
  for(i = 1; i < v; i *= 2);
  return i;
}

GLuint load_char_texture(FT_Face face, uint utf32)
{
  uchar* buffer;
  int w;
  int h;
  int texDim;
  FT_Bitmap bmp;
  int x;
  int y;
  int i;
  GLuint textureHandle;

  FT_Load_Char(face, utf32, FT_LOAD_RENDER);
  bmp = face->glyph->bitmap;
  w = bmp.width;
  h = bmp.rows;
  texDim = get_appropriate_power_of_two(max(w, h));
  buffer = malloc(texDim * texDim);
  memset(buffer, 0, texDim * texDim);
  
  x = (texDim - w) / 2;
  y = (texDim - h) / 2;
  for(i = 0; i < h; i++)
    {
      memcpy(&buffer[texDim * (i + y) + x], &bmp.buffer[w * i], w);
    }
  
  glGenTextures(1, &textureHandle);
  glBindTexture(GL_TEXTURE_2D, textureHandle);
  glTexImage2D(GL_TEXTURE_2D, 0,
               GL_RED, texDim, texDim,
               0, GL_RED, GL_UNSIGNED_BYTE, buffer);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);
  free(buffer);
  return textureHandle;
}

void show_gl_shader_compilation_error(GLuint shaderHandle)
{
  int errorLogLength;
  glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &errorLogLength);
  char* buffer = malloc(errorLogLength + 1);
  glGetShaderInfoLog(shaderHandle, errorLogLength + 1, NULL, buffer);
  fprintf(stderr, buffer);
  free(buffer);
}

void show_gl_linking_error(GLuint programHandle)
{
  int errorLogLength;
  glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &errorLogLength);
  char* buffer = malloc(errorLogLength + 1);
  glGetProgramInfoLog(programHandle, errorLogLength + 1, NULL, buffer);
  fprintf(stderr, buffer);
  free(buffer);
}

void clean_up(GLFWwindow* win)
{
  glfwDestroyWindow(win);
  glfwTerminate();
}

void init_glew()
{
  glewInit();
}

void create_texture_for_chars()
{
  FT_Library lib;
  FT_Face face;
  int i;
  
  FT_Init_FreeType(&lib);
  FT_New_Face( lib, FONTPATH, 0, &face);
  FT_Set_Char_Size(face, 0, 256*64, 100, 100);
  for(i = 0; i < CHARCOUNT; i++)
    {
      charTextures[i] = load_char_texture(face, chars[i]);
    }
  FT_Done_FreeType(lib);
}

/* returns shader program id*/
GLuint setup_shaders()
{
  GLuint programHandle;
  GLuint vsHandle;
  GLuint fgHandle;
  int compilationStatus;

  vsHandle = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vsHandle, 1, &vertexShader, NULL);
  glCompileShader(vsHandle);
  glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compilationStatus);
  if (compilationStatus != GL_TRUE)
    {
      fprintf(stderr, "VERTEX SHADER ERROR\n");
      show_gl_shader_compilation_error(vsHandle);
    }

  fgHandle = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fgHandle, 1, &fragShader, NULL);
  glCompileShader(fgHandle);
  glGetShaderiv(fgHandle, GL_COMPILE_STATUS, &compilationStatus);
  if (compilationStatus != GL_TRUE)
    {
      fprintf(stderr, "FRAG SHADER ERROR\n");
      show_gl_shader_compilation_error(fgHandle);
    }
  
  programHandle = glCreateProgram();
  glAttachShader(programHandle, vsHandle);
  glAttachShader(programHandle, fgHandle);
  glLinkProgram(programHandle);
  glGetProgramiv(programHandle, GL_LINK_STATUS, &compilationStatus);
  if (compilationStatus != GL_TRUE)
    {
      fprintf(stderr, "ERROR: while linking shader\n");
      show_gl_linking_error(programHandle);
      return -1;
    }
  glUseProgram(programHandle);

  return programHandle;
}

void setup_gl(GLuint* vbHandle, GLuint* uvHandle, GLuint* idxHandle,
              GLuint* vbVar, GLuint* uvVar, GLuint* texVar)
{
  GLuint programHandle;
  GLuint backColorVar, foreColorVar;
  
  programHandle = setup_shaders();
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  
  *vbVar = glGetAttribLocation(programHandle, "vertexPosition");
  *uvVar = glGetAttribLocation(programHandle, "vertexUV");
  *texVar = glGetUniformLocation(programHandle, "myTexture");
  backColorVar = glGetUniformLocation(programHandle, "backColor");
  foreColorVar = glGetUniformLocation(programHandle, "foreColor");
  glUniform3f(backColorVar, BACK_R / 255.0f, BACK_G / 255.0f, BACK_B / 255.0f);
  glUniform3f(foreColorVar, FORE_R / 255.0f, FORE_G / 255.0f, FORE_B / 255.0f);
  
  /* Vertex buffer */
  glGenBuffers(1, vbHandle);
  glBindBuffer(GL_ARRAY_BUFFER, *vbHandle);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  /* UV buffer*/
  glGenBuffers(1, uvHandle);
  glBindBuffer(GL_ARRAY_BUFFER, *uvHandle);
  glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);
  
  /* Index buffer*/
  glGenBuffers(1, idxHandle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *idxHandle);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vboIdx), vboIdx, GL_STATIC_DRAW);
}

void key_event(GLFWwindow* win, int key, int scan, int action, int mods)
{
  if (action != GLFW_RELEASE)
    return;
  if (key == GLFW_KEY_UP || key == GLFW_KEY_LEFT)
    {
      curTextureIdx -= 1;
      if (curTextureIdx < 0)
        curTextureIdx = CHARCOUNT - 1;
    }
  else if(key == GLFW_KEY_DOWN || key == GLFW_KEY_RIGHT)
    {
      curTextureIdx += 1;
      if (curTextureIdx >= CHARCOUNT)
        curTextureIdx = 0;
    }
}

GLFWwindow* create_window()
{
  GLFWwindow* win;

  glfwInit();
  win = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Draw text", NULL, NULL);
  glfwSetKeyCallback(win, key_event);
  glfwMakeContextCurrent(win);
  return win;
}

int main()
{
  GLFWwindow* win;
  int curW = DEFAULT_WIDTH;
  int curH = DEFAULT_HEIGHT;
  int lastW = 0;
  int lastH = 0;
  int minDim = 0;
  uint vbHandle, uvHandle, idxHandle;
  uint vbVar, uvVar,  texVar;

  win = create_window();
  init_glew();
  create_texture_for_chars();
  setup_gl(&vbHandle, &uvHandle, &idxHandle, 
           &vbVar, &uvVar, &texVar);

  glEnableVertexAttribArray(vbVar);
  glEnableVertexAttribArray(uvVar);
  
  while(1)
    {
      glfwGetWindowSize(win, &curW, &curH);
      if(curW != lastW || curH != lastH)
        {
          minDim = min(curW, curH);
          lastW = curW;
          lastH = curH;
          glViewport((curW - minDim) / 2, (curH - minDim) / 2, minDim, minDim);
        }

      glClear(GL_COLOR_BUFFER_BIT);
      
      glBindBuffer(GL_ARRAY_BUFFER, vbHandle);
      glVertexAttribPointer(vbVar, 3, GL_FLOAT,
                            GL_FALSE, 0, NULL);
      
      glBindBuffer(GL_ARRAY_BUFFER, uvHandle);
      glVertexAttribPointer(uvVar, 2, GL_FLOAT,
                            GL_FALSE, 0, NULL);
      
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxHandle);
      glBindTexture(GL_TEXTURE_2D, charTextures[curTextureIdx]);
      
      glUniform1i(texVar, 0);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

      glFlush();

      glfwSwapBuffers(win);
      glfwPollEvents();
      if (glfwWindowShouldClose(win))
        {
          break;
        }
    }
  
  clean_up(win);
  return 0;
}
