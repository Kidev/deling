/****************************************************************************
 ** OpenGL to RHI Migration Adapter
 ** Copyright (C) 2025 Alexandre 'kidev' Poumaroux
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

#include <QtCore/qglobal.h>
#include <QtWidgets>
#include <QMatrix4x4>

#ifdef USE_RHI

#include "3d/WorldmapRHIWidget.h"
#include "3d/WalkmeshRHIWidget.h"

using WorldmapGLWidget = WorldmapRHIWidget;
using WalkmeshGLWidget = WalkmeshRHIWidget;

using GLfloat = float;
using GLint = int;
using GLuint = unsigned int;

#else

#include "3d/WorldmapGLWidget.h"
#include "3d/WalkmeshGLWidget.h"
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#endif
