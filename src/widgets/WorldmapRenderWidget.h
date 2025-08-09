/****************************************************************************
 * * Deling QRhi port
 ** Copyright (C) 2025 Alexandre 'kidev' Poumaroux <public@kidev.org>
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
// WARNING: UNUSED IN PRACTICE, WORK IN PROGRESS, DO NOT OPEN MAPS

#pragma once

#include "WalkmeshRenderWidget.h"

class WorldmapRenderWidget : public WalkmeshRenderWidget
{
	Q_OBJECT
public:
	WorldmapRenderWidget(QWidget *parent = nullptr) : WalkmeshRenderWidget(parent) {}
	void setMap(Map* map) {Q_UNUSED(map);}
	void setXTrans(float value) {Q_UNUSED(value);}
	void setYTrans(float value) {Q_UNUSED(value);}
	void setZTrans(float value) {Q_UNUSED(value);}
	void setXRot(float value) {Q_UNUSED(value);}
	void setYRot(float value) {Q_UNUSED(value);}
	void setZRot(float value) {Q_UNUSED(value);}
	float xRot() const {return 0.0f;}
	float yRot() const {return 0.0f;}
	float zRot() const {return 0.0f;}
};
