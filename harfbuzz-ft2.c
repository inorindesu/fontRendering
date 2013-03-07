/* 
 * Copyright (C) 2013  Inori Sakura <inorindesu@gmail.com>
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file for more details.
 */

#include <hb.h>
#include <hb-glib.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <png.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

typedef unsigned char uchar;
typedef unsigned int uint;

void my_write(png_structp ps, png_bytep data, png_size_t sz)
{
  fwrite(data, 1, sz, stdout);
}

void my_flush(png_structp ps)
{
  fflush(stdout);
}

void write_png(uchar* data, int w, int h)
{
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  /* depth parameter means depth-per-channel*/
  png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE
               , PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_set_write_fn(png, NULL, my_write, my_flush);

  uchar** rows = malloc(sizeof(uchar*) * h);
  int i;
  for(i = 0; i < h; i++)
    {
      rows[i] = &data[i * w * 4];
    }

  png_write_info(png, info);
  png_write_image(png, rows);
  free(rows);
  png_destroy_write_struct(&png, &info);
}

uchar* read_all_mmap(char* path, int* fileSize)
{
  struct stat statBuf;
  
  /* Some little check.. */
  if(stat(path, &statBuf) != 0)
    {
      return NULL;
    }
  else if(!S_ISREG(statBuf.st_mode))
    {
      fprintf(stderr, "WARNING: not a file\n");
      return NULL;
    }
  
  *fileSize = statBuf.st_size;
  int fd = open(path, 0);
  uchar* buf = mmap(NULL, *fileSize, PROT_READ, MAP_SHARED, fd, 0);
  if (buf == NULL)
    {
      fprintf(stderr, "WARNING: mmap failed\n");
      return NULL;
    }
  close(fd);
  return buf;
}

void free_mmap(uchar* buf, int len)
{
  munmap(buf, len);
}

int main(int argc, char** argv)
{
  if (argc < 3)
    {
      fprintf(stderr, "USAGE: harfbuzz-ft2 [fontfile] [text]\n");
      return 0;
    }

  int dataSize;
  uchar* data = read_all_mmap(argv[1], &dataSize);
  if (data == NULL)
    {
      fprintf(stderr, "There's some problems while reading font file..\n");
      return -1;
    }
  
  /* setup font */
  fprintf(stderr, "Loading font: %s\n", argv[1]);
  hb_blob_t* blob = hb_blob_create(data, dataSize,
                                   HB_MEMORY_MODE_READONLY,
                                   NULL,
                                   NULL);

  hb_face_t* face = hb_face_create(blob, 0);
  hb_font_t* font = hb_font_create(face);
  uint upem = hb_face_get_upem(face);
  upem *= 5;
  fprintf(stderr, "UPEM of this font: %u\n", upem);
  fprintf(stderr, "Estimated font height (in pixel): %u\n", upem / 64);
  hb_font_set_scale(font, upem, upem);
  hb_ft_font_set_funcs (font);
  
  /* prepare the buffer */
  hb_buffer_t* buffer = hb_buffer_create();
  hb_unicode_funcs_t* unicodeFuncs = hb_glib_get_unicode_funcs();
  hb_buffer_set_unicode_funcs(buffer, unicodeFuncs);
  hb_buffer_add_utf8(buffer, argv[2], strlen(argv[2]), 0, strlen(argv[2]));
  hb_buffer_guess_segment_properties(buffer);
  
  /* shaping */
  uint infoLen, posLen;
  hb_shape(font, buffer, NULL, 0);
  hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(buffer, &infoLen);
  hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(buffer, &posLen);
  /*
   * Problem:
   * How can I clean glyphInfo and glyphPos??
   * They will be cleaned when hb_buffer_t destroyed??
   */
  
  /* print shaping result */
  fprintf(stderr, "%u glyph infos, %u glyph positions.\n", infoLen, posLen);
  int i;
  if(infoLen != posLen)
    {
      fprintf(stderr, "WARNING: infoLen != posLen!!\n");
    }
  for(i = 0; i < infoLen; i++)
    {
      fprintf(stderr, "Codepoint: %u\n", glyphInfo[i].codepoint);
      fprintf(stderr, "Cluster: %u\n", glyphInfo[i].cluster);
      fprintf(stderr, "X advance: %d\n", glyphPos[i].x_advance);
      fprintf(stderr, "Y advance: %d\n", glyphPos[i].y_advance);
      fprintf(stderr, "X offset: %d\n", glyphPos[i].x_offset);
      fprintf(stderr, "Y offset: %d\n\n", glyphPos[i].y_offset);
    }
  
  /*
   * bouncing box
   * In current implementation, \n will be ignored.
   *
   * A better way to estimate boundary is do pseudorendering
   * on all glyphs.
   */
  FT_Face ftFace = hb_ft_font_get_face(font);
  int w = 0;
  int h = ftFace->size->metrics.height / 64 + 2;
  h *= 1.5; /* make more room for arabic*/
  int descender = ftFace->size->metrics.descender / 64;
  if (descender < 0)
    {
      descender = descender * (-1) + 1;
    }
  else
    {
      descender += 1;
    }
  for(i = 0; i < infoLen; i++)
    {
      w += glyphPos[i].x_advance;
      /* I'm not using vertical layout.
       * So I think there will be no y_advance.*/
    }
  w /= 64;
  fprintf(stderr, "Bound: %u, %u\n", w, h);
  
  /*
   * Rendering
   * Bug:
   * Font height is not a good measure for bounding box in arabic.
   * (May use pseudorendering)
   */
  uchar* imgData = calloc(1, w * h * 4);
  int x26_6 = 0;
  int y26_6 = (h - descender) * 64;
  for(i = 0; i < infoLen; i++)
    {
      int j, k;
      FT_Load_Glyph(ftFace, glyphInfo[i].codepoint, FT_LOAD_DEFAULT);
      FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL);
      FT_Bitmap* bmp = &ftFace->glyph->bitmap;
      int penY = y26_6 - ftFace->glyph->bitmap_top * 64 - glyphPos[i].y_offset;
      penY = penY / 64;
      int penX = x26_6 + ftFace->glyph->bitmap_left * 64 + glyphPos[i].x_offset;
      penX = penX / 64;
      for(j = 0; j < bmp->rows; j++)
        {
          for(k = 0; k < bmp->width; k++)
            {
              int imgPos = (w * (j + penY) + penX + k) * 4;
              int glyphBmpPos = bmp->width * j + k;
              if (imgPos < 0 || imgPos + 4 > w * h * 4)
                continue;
              if (glyphBmpPos < 0 || glyphBmpPos >= bmp->width * bmp->rows)
                continue;
              if (imgData[imgPos + 3] < bmp->buffer[glyphBmpPos])
                  imgData[imgPos + 3] = bmp->buffer[glyphBmpPos];
            }
        }
      x26_6 += glyphPos[i].x_advance;
    }
  write_png(imgData, w, h);
  free(imgData);
  
  /* cleanup */
  /*
   * FT_Done_Face is not needed, since harfbuzz will do that.
   */
  hb_unicode_funcs_destroy(unicodeFuncs);
  hb_buffer_destroy(buffer);
  hb_font_destroy(font);
  hb_face_destroy(face);
  hb_blob_destroy(blob);
  free_mmap(data, dataSize);
  return 0;
}
