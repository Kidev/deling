/****************************************************************************
 ** Makou Reactor Final Fantasy VII Field Script Editor
 ** Copyright (C) 2009-2024 Arzel Jérôme <myst6re@gmail.com>
 ** Copyright (C) 2020 Julian Xhokaxhiu <https://julianxhokaxhiu.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#pragma once

#include <QMatrix4x4>
#include <QObject>
#include <QColor>
#include <QRgba64>
#include <QVector3D>
#include <QVector2D>
#include <QWidget>
#include <QImage>
#include <vector>
#include <cstdint>

#ifndef NO_OPENGL_WIDGETS
#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

// OpenGL types
typedef float GLfloat;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef unsigned int GLuint;

#else
// Fallback types when OpenGL is not available
typedef float GLfloat;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef unsigned int GLuint;

// Mock OpenGL constants
#ifndef GL_POINTS
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_QUAD_STRIP 0x0008
#define GL_POLYGON 0x0009
#endif

#endif

struct RendererVertex {
	GLfloat position[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
	GLfloat color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat texcoord[2]{ 0.0f, 0.0f };
};

enum RendererPrimitiveType {
	PT_POINTS = GL_POINTS,
	PT_LINES = GL_LINES,
	PT_LINE_LOOP = GL_LINE_LOOP,
	PT_LINE_STRIP = GL_LINE_STRIP,
	PT_TRIANGLES = GL_TRIANGLES,
	PT_TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
	PT_TRIANGLE_FAN = GL_TRIANGLE_FAN,
	PT_QUADS = GL_QUADS,
	PT_QUAD_STRIP = GL_QUAD_STRIP,
	PT_POLYGON = GL_POLYGON
};

class Renderer : public QObject
{
	Q_OBJECT
private:
#ifndef NO_OPENGL_WIDGETS
	enum ShaderProgramAttributes{ POSITION = 0, COLOR, TEXCOORD };

	QOpenGLWidget *mWidget;

	QOpenGLFunctions mGL;

	QOpenGLShaderProgram mProgram;
	QOpenGLShader mVertexShader;
	QOpenGLShader mFragmentShader;

	QOpenGLVertexArrayObject mVAO;

	QOpenGLBuffer mVertex;
	QOpenGLBuffer mIndex;

	QOpenGLTexture mTexture;
#ifdef QT_DEBUG
	QOpenGLDebugLogger mLogger;
#endif
#else
	QWidget *mWidget;
#endif

	std::vector<RendererVertex> mVertexBuffer;
	std::vector<uint32_t> mIndexBuffer;

	bool _hasError;

	QMatrix4x4 mModelMatrix;
	QMatrix4x4 mProjectionMatrix;
	QMatrix4x4 mViewMatrix;

public:
#ifndef NO_OPENGL_WIDGETS
	Renderer(QOpenGLWidget *_widget);
#else
	Renderer(QWidget *_widget);
#endif

	inline bool hasError() const
	{
		return _hasError;
	}
	void clear();
	void show();
	void reset();

	void draw(RendererPrimitiveType _type, float _pointSize = 1.0f,
	          bool clear = true);

	void setViewport(int32_t _x, int32_t _y, int32_t _width, int32_t _height);

	void bindModelMatrix(QMatrix4x4 _matrix);
	void bindProjectionMatrix(QMatrix4x4 _matrix);
	void bindViewMatrix(QMatrix4x4 _matrix);

	void bindVertex(const RendererVertex *_vertex, uint32_t _count = 1);
	void bindIndex(uint32_t *_index, uint32_t _count = 1);

	void bindTexture(const QImage &_image, bool generateMipmaps = false);
#ifndef NO_OPENGL_WIDGETS
	void bindTexture(QOpenGLTexture *texture);
#else
	void bindTexture(void *texture); // Dummy parameter for compatibility
#endif

	void bufferVertex(QVector3D _position, QRgba64 _color, QVector2D _texcoord);

#ifndef NO_OPENGL_WIDGETS
#ifdef QT_DEBUG
protected slots:
	void messageLogged(const QOpenGLDebugMessage &msg);
#endif
private:
	bool updateBuffers();
	void drawStart(float _pointSize);
	void drawEnd(bool clear);
#endif
	bool _buffersHaveChanged;
};