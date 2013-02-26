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


#include <ft2build.h>
#include FT_FREETYPE_H

void render_glyph_to_stdout(FT_GlyphSlot slot)
{
  /*
   * The PNG file would be rendered in RGBA color format.
   */
}

int main(int argc, char** argv)
{
  FT_Library lib;
  FT_Face face;
  FT_Error err;
  char charToRender;
  int i;
  unsigned int glyphIndex;
  FT_GlyphSlot glyphSlot;

  if (argc != 3)
    {
      printf("Usage: %s char fontPath\nExample: %s G /usr/share/fonts/gnu-free/FreeSans.ttf\n", argv[0], argv[0]);
      return 0;
    }

  charToRender = argv[1][0];
  
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
  
  /* Show font information*/
  printf("Font loaded from %s.\n", argv[2]);
  printf("Family name: %s.\n", face->family_name);
  printf("Style name: %s.\n", face->style_name);
  printf("# of faces in this font: %d\n", face->num_faces);
  printf("BBox: x: (%d, %d), y: (%d, %d)\n", face->bbox.xMin, face->bbox.xMax,
         face->bbox.yMin, face->bbox.yMax);
  printf("Units per EM: %d\n", face->units_per_EM);
  printf("Ascender: %d\n", face->ascender);
  printf("Descender: %d\n", face->descender);
  printf("(Line) Height: %d\n", face->height);
  
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
  printf("Try loading character %c\n", charToRender);
  glyphIndex = FT_Get_Char_Index(face, charToRender);
  if (glyphIndex == 0)
    {
      printf("The character cannot be indexed.\n");
      return 0;
    }
  else
    {
      printf("Glyph index of %c is %d\n", charToRender, glyphIndex);
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
      
      printf("Rendering PNG with cairo.");
      render_glyph_to_stdout(glyphSlot);
    }
  FT_Done_Face(face);
  FT_Done_FreeType(lib);
  return 0;
}







