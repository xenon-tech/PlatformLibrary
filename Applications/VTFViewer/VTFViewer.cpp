/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "platform_image.h"
#include "platform_window.h"
#include "platform_graphics.h"
#include "platform_log.h"

#include <GLFW/glfw3.h>

// The following is an example of loading a VTF texture using the platform libraries image functionality.

using namespace pl;

#define TITLE   "VTF/VMT Viewer"
#define LOG     "vtfviewer"

int main(int argc, char *argv[]) {
    plClearLog(LOG);

    if(!glfwInit()) {
        plMessageBox(TITLE, "Failed to initialize GLFW!\n");
        return -1;
    }

    GLFWwindow *window = glfwCreateWindow(640, 480, TITLE, NULL, NULL);
    if(!window) {
        glfwTerminate();

        plMessageBox(TITLE, "Failed to create window!\n");
        return -1;
    }

    glfwMakeContextCurrent(window);

    plInitialize(PL_SUBSYSTEM_GRAPHICS);

    plSetDefaultGraphicsState();

    plEnableGraphicsStates(PL_CAPABILITY_DEPTHTEST);

    plSetClearColour(plCreateColour4b(PL_COLOUR_RED));

    // Set up the viewport.
    plViewport(0, 0, 640, 480);
    plScissor(0, 0, 640, 480);

    // Load the image up from the HDD.
    PLImage image;
    PLresult result = plLoadImage("./images/bluegrid.vtf", &image);
    if(result != PL_RESULT_SUCCESS) {
        plMessageBox(TITLE, "Failed to load VTF!\n%s", plGetResultString(result));
        return -1;
    }

    // Create a texture slot for our new texture.
    PLTexture *image_texture = plCreateTexture();
    if(!image_texture) {
        plMessageBox(TITLE, "Failed to create texture!");
        return -1;
    }

    // Assign that image to our texture and upload it to the GPU.
    plSetTextureFilter(image_texture, PL_TEXTUREFILTER_NEAREST);
    plUploadTextureImage(image_texture, &image);

    PLMesh *cube = plCreateMesh(PL_PRIMITIVE_TRIANGLE_FAN, PL_DRAW_STATIC, 2, 4);
    if(!cube) {
        plMessageBox(TITLE, "Failed to create mesh!\n%s", plGetError());
        return -1;
    }

    while(!glfwWindowShouldClose(window)) {
        plClearBuffers(PL_BUFFER_COLOUR | PL_BUFFER_DEPTH | PL_BUFFER_STENCIL);

        // draw stuff

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    plDeleteTexture(image_texture, true);
    plDeleteMesh(cube);

    glfwTerminate();

    plShutdown();

    return 0;
}