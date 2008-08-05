//===========================================================================
/*
    This file is part of the CHAI 3D visualization and haptics libraries.
    Copyright (C) 2003-2004 by CHAI 3D. All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.

    For using the CHAI 3D libraries with software that can not be combined
    with the GNU GPL, and for taking advantage of the additional benefits
    of our support services, please contact CHAI 3D about acquiring a
    Professional Edition License.

    \author:    <http://www.chai3d.org>
    \author:    Dan Morris
    \author:    Francois Conti
    \version    1.0
    \date       03/2004
*/
//===========================================================================

//---------------------------------------------------------------------------
#ifndef CImageLoaderH
#define CImageLoaderH
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------

// Global utility functions:

//! Finds the extension in a filename and returns a pointer to the character after the '.'
char* find_extension(const char* a_input);

//! Converts a string to lower-case
void string_tolower(char* a_dest,const char* a_source);

//! Finds only the _path_ portion of source, and copies it to dest
void find_directory(char* a_dest, const char* a_source);


//===========================================================================
/*!
      \class    cImageLoader
      \brief    cImageLoader provides a class to load images files
                into memory.  The real work is deferred to specific files
                that know how to load specific image file types.
*/
//===========================================================================
class cImageLoader
{
  public:

    //! Default constructor; doesn't load anything...
    cImageLoader();

    //! Default constructor; loads the specified filename...
    cImageLoader(const char* filename);

    //! Destructor of cImageLoader
    virtual ~cImageLoader();

    //! Get a pointer to the actual image data... use with care...
    inline unsigned char* getData() { return m_data;   }

    //! Get width of image.
    inline unsigned int getWidth()  { return m_width;  }

    //! Get height of image.
    inline unsigned int getHeight() { return m_height; }

    //! Get the format (GL_RGB or GL_RGBA) of the image
    inline unsigned int getFormat() { return m_format; }

    //! Get the number of bits per pixel used to store this image
    inline unsigned int getBitsPerPixel() { return m_bits_per_pixel; }

    //! Returns 1 if a file has been successfully loaded, 0 otherwise
    inline unsigned int initialized() { return m_initialized; }

    //! Load image file by passing image path and name as argument
    int loadFromFile(const char* filename);

  protected:

    //! Initialize member variables
    void defaults();

    //! Delete memory and rid ourselves of any image we had previously stored
    void cleanup();

    //! The last image filename that I loaded
    char m_filename[_MAX_PATH];

    //! The dimensions of the current image
    int m_width, m_height;

    //! Either GL_RGB or GL_RGBA
    unsigned int m_format;

    //! Basically always 8...
    unsigned int m_bits_per_pixel;

    //! All images are converted from their native format to RGBA by this class
    void convertToRGBA();

    //! The image data itself
    unsigned char* m_data;

    //! Have I actually loaded a valid image?
    bool m_initialized;
};

#endif