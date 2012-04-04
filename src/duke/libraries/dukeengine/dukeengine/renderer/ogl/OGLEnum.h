#ifndef OGLENUM_H_
#define OGLENUM_H_

#include <GL/glew.h>
#include <dukeengine/renderer/Enums.h>
#include <dukeio/Formats.h>

class OGLEnum
{
public:
	static unsigned long BufferFlags( unsigned long Flags );
	static unsigned long LockFlags( unsigned long Flags );
	static GLint         GetFormat( TPixelFormat format );
	static GLint         GetInternalFormat( TPixelFormat format );
	static GLint         GetType( TPixelFormat format );
};

#endif /* OGLENUM_H_ */
