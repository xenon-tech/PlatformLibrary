/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "image_private.h"

/*	Ritual's FTX Format	*/

typedef struct FtxHeader {
	uint32_t width;
	uint32_t height;
	uint32_t alpha;
} FtxHeader;

PLImage *PlLoadFtxImage( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	FtxHeader header;
	bool status;
	header.width = PlReadInt32( file, false, &status );
	header.height = PlReadInt32( file, false, &status );
	header.alpha = PlReadInt32( file, false, &status );

	if ( !status ) {
		PlCloseFile( file );
		return NULL;
	}

	unsigned int size = header.width * header.height * 4;
	uint8_t *buffer = pl_malloc( size );
	size_t rSize = PlReadFile( file, buffer, sizeof( uint8_t ), size );

	PlCloseFile( file );

	if ( rSize != size ) {
		pl_free( buffer );
		return NULL;
	}

	PLImage *image = PlCreateImage( buffer, header.width, header.height, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );

	/* create image makes a copy of the buffer */
	pl_free( buffer );

	return image;
}
