/* 
 * Render single character from selected font, then output the result into
 * stdout.
 *
 * Usage:
 * ft2_char character fontPath > output.png
 *
 * Example:
 * ft2_char M /usr/share/fonts/gnu-free/FreeSans.ttf
 * 
 * Copyright (C) 2013  Inori Sakura <inorindesu@gmail.com>
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file for more details.
 */

#include <iconv.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cairo.h>

#include <ft2build.h>
#include FT_FREETYPE_H

cairo_status_t my_writer(void* closure, const unsigned char *data, unsigned int length)
{
  if (fwrite(data, 1, length, stdout) != length)
    {
      fprintf(stderr, "ERROR: writing error");
      return CAIRO_STATUS_WRITE_ERROR;
    }
  return CAIRO_STATUS_SUCCESS;
}

/*
 * Render FreeType glyph into PNG file, in ARGB format, delivering to stdout.
 * 
 * Change the following defines to change rendering color.
 */
# define RED (192)
# define GREEN (255)
# define BLUE (192)
void render_glyph_to_stdout(FT_GlyphSlot slot)
{
  cairo_surface_t* img;
  unsigned char* imgData;
  FT_Bitmap* bitmap;
  int i;
  
  bitmap = &slot->bitmap;
  /*
   * Prepare image data for cairo PNG
   *
   * ARGB32 in cairo is in pre-multiplied alpha format
   * The byte sequences are: BGRA
   */

  imgData = malloc(bitmap->width * bitmap->rows * 4);
  for(i = 0; i < bitmap->width * bitmap->rows; i++)
    {
      int imgIndex = i * 4;
      unsigned char alpha = bitmap->buffer[i];
      imgData[imgIndex] = BLUE * alpha / 255;/*b*/
      imgData[imgIndex + 1] = GREEN * alpha / 255; /*g*/
      imgData[imgIndex + 2] = RED * alpha / 255; /*r*/
      imgData[imgIndex + 3] = alpha; /*a*/
    }

  /*
   * create a surface from the data
   */
  img = cairo_image_surface_create_for_data(imgData, CAIRO_FORMAT_ARGB32, bitmap->width, bitmap->rows, bitmap->width * 4);
  cairo_surface_write_to_png_stream(img, my_writer, NULL);
  cairo_surface_destroy(img);
  free(imgData);
}

/*
 * Convert the first character of a given utf8 string into a utf32
 * character.
 */
#define UTF32_BUF_LEN 8  /* 4 for BOM, 4 for character itself*/
unsigned int utf8_to_utf32(char* utf8)
{
  iconv_t t;
  char* buf;
  char* originalBuf;
  unsigned int ret;
  int converted;
  size_t inSize;
  size_t outSize;
  int i;

  /*
   * from utf8 to utf32
   *
   * Using 'utf-32' as the first parameter would cause iconv write BOM
   * into output stream.
   */
  t = iconv_open("UTF-32", "UTF-8");
  if (t == (iconv_t) -1)
    {
      fprintf(stderr, "ERROR: cannot create converter.");
      return 0;
    }

  buf = malloc(UTF32_BUF_LEN);
  memset(buf, 0, UTF32_BUF_LEN);

  originalBuf = buf;
  inSize = strlen(utf8);
  outSize = UTF32_BUF_LEN;
  converted = iconv(t, &utf8, &inSize, &buf, &outSize);

  if (converted == (size_t) -1)
    {
      if (errno != E2BIG)
        {
          if (errno == EILSEQ)
            {
              fprintf(stderr, "WARNING: EILSEQ\n");
            }
          else if (errno == EINVAL)
            {
              fprintf(stderr, "WARNING: EINVAL\n");
            }
          else
            {
              fprintf(stderr, "WARNING: converted character count is %d\n", converted);
            }
        }
    }

  /* Ignore the BOM */
  ret = ((unsigned int*)originalBuf)[1];
  free(originalBuf);
  return ret;
}

/*
 * Fetch first utf8 char from a utf8 string. Returned char array
 * should be freed by free() after their jobs done.
 *
 * UTF-8 rule: see http://zh.wikipedia.org/wiki/UTF-8
 */
char* first_utf8_code_new(char* str)
{
  char c = str[0];
  char* buf;
  int codeLength;

  if (c & 0b10000000 == 0)
    {
      codeLength = 1;
    }
  else if (c & 0b11100000 == 0b11000000)
    {
      codeLength = 2;
    }
  else if (c & 0b11110000 == 0b11100000)
    {
      codeLength = 3;
    }
  else 
    {
      codeLength = 4;
    }
  buf = malloc(codeLength + 1);
  memcpy(buf, str, codeLength);
  buf[codeLength] = '\0';
  return buf;
}

int main(int argc, char** argv)
{
  FT_Library lib;
  FT_Face face;
  FT_Error err;
  char* charToRender;
  unsigned int utf32CharToRender;
  int i;
  unsigned int glyphIndex;
  FT_GlyphSlot glyphSlot;

  if (argc != 3)
    {
      printf("Usage: %s char fontPath\nExample: %s G /usr/share/fonts/gnu-free/FreeSans.ttf\n", argv[0], argv[0]);
      return 0;
    }

  /* Initiate freetype library */
  err = FT_Init_FreeType(&lib);
  if (err)
    {
      fprintf(stderr, "ERROR: init library");
      return -1;
    }

  /* Load a font from a font file.*/
  err = FT_New_Face(lib, argv[2], 0, &face);
  if (err == FT_Err_Unknown_File_Format)
    {
      fprintf(stderr, "ERROR: unrecognized font format");
      return -1;
    }
  else if (err)
    {
      fprintf(stderr, "ERROR: when loading font");
      return -1;
    }
  
  /*
   * Show font information
   * Direct them to stderr, since we write final PNG image into stdout.
   */
  fprintf(stderr, "Font loaded from %s.\n", argv[2]);
  fprintf(stderr, "Family name: %s.\n", face->family_name);
  fprintf(stderr, "Style name: %s.\n", face->style_name);
  fprintf(stderr, "# of faces in this font: %d\n", face->num_faces);
  fprintf(stderr, "BBox: x: (%d, %d), y: (%d, %d)\n", face->bbox.xMin, face->bbox.xMax,
          face->bbox.yMin, face->bbox.yMax);
  fprintf(stderr, "Units per EM: %d\n", face->units_per_EM);
  fprintf(stderr, "Ascender: %d\n", face->ascender);
  fprintf(stderr, "Descender: %d\n", face->descender);
  fprintf(stderr, "(Line) Height: %d\n", face->height);
  
  /*
   * Set size of the font to width or height
   * (setting only one of them is OK)
   * Parameters are: FT_Font, width, height, xdpi, ydpi
   */
  err = FT_Set_Char_Size(face, 0, 64 * 64, 100, 100);
  if (err)
    {
      fprintf(stderr, "ERROR: setting font size\n");
      return -1;
    }

  /*
   *    Get character index in the font
   * -> Load the glyph from given index
   * -> Render loaded glyph into a bitmap
   *
   * charToRender should be a utf-32 character
   * if default charmap is a unicode charmap
   */
  charToRender = first_utf8_code_new(argv[1]);
  utf32CharToRender = utf8_to_utf32(charToRender);
  
  fprintf(stderr, "Try loading character %s (UTF32 code: %u)\n", charToRender, utf32CharToRender);
  glyphIndex = FT_Get_Char_Index(face, utf32CharToRender);
  if (glyphIndex == 0)
    {
      fprintf(stderr, "The character cannot be indexed.\n");
      return 0;
    }
  else
    {
      fprintf(stderr, "Glyph index of %s is %d\n", charToRender, glyphIndex);
      err = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT /*0*/);
      if (err)
        {
          fprintf(stderr, "ERROR: when loading glyph\n");
          return -1;
        }
      
      glyphSlot = face->glyph;
      err = FT_Render_Glyph(glyphSlot, FT_RENDER_MODE_NORMAL /*0*/);
      if (err)
        {
          fprintf(stderr, "ERROR: when rendering glyph\n");
          return -1;
        }

      fprintf(stderr, "Glyph information of character %s:", charToRender);
      
      fprintf(stderr, "Rendering PNG with cairo.\n");
      render_glyph_to_stdout(glyphSlot);
    }
  FT_Done_Face(face);
  FT_Done_FreeType(lib);
  free(charToRender);
  return 0;
}
