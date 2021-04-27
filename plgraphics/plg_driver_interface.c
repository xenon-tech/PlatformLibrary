/*
MIT License

Copyright (c) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/*===========================
	DRIVER INTERFACE
===========================*/

#include <plcore/pl_linkedlist.h>
#include <plgraphics/plg_driver_interface.h>

#include "plg_private.h"

#define DRIVER_EXTENSION "_driver"

#define MAX_GRAPHICS_MODES 16

typedef struct PLGDriver {
	char identifier[ 32 ];
	char description[ 64 ];

	PLLibrary *libPtr; /* library handle */
	const PLGDriverImportTable *interface;
	PLGDriverInitializationFunction initFunction; /* initialization function */
} PLGDriver;

static PLGDriver drivers[ MAX_GRAPHICS_MODES ];
static unsigned int numDrivers = 0;

static PLGDriverExportTable exportTable = {
		.CreateTexture = PlgCreateTexture,
		.DestroyTexture = PlgDestroyTexture,
		.GetMaxTextureAnistropy = PlgGetMaxTextureAnistropy,
		.GetMaxTextureSize = PlgGetMaxTextureSize,
		.GetMaxTextureUnits = PlgGetMaxTextureUnits,
		.SetTexture = PlgSetTexture,
		.SetTextureAnisotropy = PlgSetTextureAnisotropy,
		.SetTextureEnvironmentMode = PlgSetTextureEnvironmentMode,
		.SetTextureFlags = PlgSetTextureFlags,
};

bool PlgRegisterDriver( const char *path ) {
	PlClearError();

	GfxLog( "Registering driver: \"%s\"\n", path );

	PLLibrary *library = PlLoadLibrary( path, false );
	if ( library == NULL ) {
		return false;
	}

	const PLGDriverDescription *description;
	PLGDriverInitializationFunction InitializeDriver;
	PLGDriverQueryFunction RegisterDriver = ( PLGDriverQueryFunction ) PlGetLibraryProcedure( library, PLG_DRIVER_QUERY_FUNCTION );
	if ( RegisterDriver != NULL ) {
		InitializeDriver = ( PLGDriverInitializationFunction ) PlGetLibraryProcedure( library, PLG_DRIVER_INIT_FUNCTION );
		if ( InitializeDriver != NULL ) {
			/* now fetch the driver description */
			description = RegisterDriver();
			if ( description != NULL ) {
				if ( description->coreInterfaceVersion[ 0 ] != PL_PLUGIN_INTERFACE_VERSION_MAJOR ) {
					PlReportErrorF( PL_RESULT_UNSUPPORTED, "unsupported core interface version" );
				} else if ( description->graphicsInterfaceVersion[ 0 ] != PLG_INTERFACE_VERSION_MAJOR ) {
					PlReportErrorF( PL_RESULT_UNSUPPORTED, "unsupported graphics interface version" );
				}
			} else {
				PlReportErrorF( PL_RESULT_FAIL, "failed to fetch driver description" );
			}
		}
	}

	if ( PlGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PlUnloadLibrary( library );
		return false;
	}

	GfxLog( "Success, adding \"%s\" to drivers list\n", path );

	PLGDriver *driver = &drivers[ numDrivers++ ];
	snprintf( driver->description, sizeof( driver->description ), "%s", description->description );
	snprintf( driver->identifier, sizeof( driver->identifier ), "%s", description->identifier );
	driver->interface = NULL;
	driver->initFunction = InitializeDriver;
	driver->libPtr = library;

	return true;
}

/**
 * Private callback when scanning for plugins.
 */
static void RegisterScannedDriver( const char *path, void *unused ) {
	plUnused( unused );

	/* validate it's actually a plugin */
	size_t length = strlen( path );
	const char *c = strrchr( path, '_' );
	if ( c == NULL ) {
		return;
	}

	/* remaining length */
	length -= c - path;
	if ( pl_strncasecmp( c, DRIVER_EXTENSION PL_SYSTEM_LIBRARY_EXTENSION, length ) != 0 ) {
		return;
	}

	PlRegisterPlugin( path );
}

void PlgScanForDrivers( const char *path ) {
	PlScanDirectory( path, NULL, RegisterScannedDriver, false, NULL );
}

/**
 * Query what available graphics modes there are so that the
 * host can either choose their preference or attempt each.
 */
const char **PlgGetAvailableDriverInterfaces( unsigned int *numModes ) {
	static unsigned int cachedModes = 0;
	static const char *descriptors[ MAX_GRAPHICS_MODES ];
	if ( cachedModes != numDrivers ) {
		for ( unsigned int i = 0; i < numDrivers; ++i ) {
			descriptors[ i ] = drivers[ i ].identifier;
		}
	}
	*numModes = numDrivers;
	cachedModes = *numModes;

	return descriptors;
}

void PlgSetDriver( const char *mode ) {
	GfxLog( "Attempting mode \"%s\"...\n", mode );

	const PLGDriverImportTable *interface = NULL;
	for ( unsigned int i = 0; i < numDrivers; ++i ) {
		if ( pl_strcasecmp( mode, drivers[ i ].identifier ) != 0 ) {
			continue;
		}

		interface = drivers[ i ].initFunction( &exportTable );
		break;
	}

	if ( interface == NULL ) {
		PlReportErrorF( PL_RESULT_GRAPHICSINIT, "invalid graphics interface \"%s\" selected", mode );
		return;
	}

	if ( gfx_state.interface != NULL && interface == gfx_state.interface ) {
		PlReportErrorF( PL_RESULT_GRAPHICSINIT, "chosen interface \"%s\" is already active", mode );
		return;
	}

	gfx_state.interface = interface;

	CallGfxFunction( Initialize );

	GfxLog( "Mode \"%s\" initialized!\n", mode );
}
