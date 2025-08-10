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
#pragma once

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QImage>
#include "Field.h"

class WalkmeshRenderWidget : public QRhiWidget
{
	Q_OBJECT
public:
	WalkmeshRenderWidget(QWidget *parent = nullptr);
	~WalkmeshRenderWidget() override;

	void setXRotation(int angle);
	void setYRotation(int angle);
	void setZRotation(int angle);
	void resetCamera();
	void setBackgroundVisible(bool show);
	void setCurrentTabIndex(int index);
	void clear();

	float xRot() const;
	float yRot() const;
	float zRot() const;

	void setCurrentFieldCamera(int camID);
	void updatePerspective();
	void setSelectedTriangle(int triangle);
	void setSelectedDoor(int door);
	void setSelectedGate(int gate);
	void setLineToDraw(const Vertex vertex[2]);
	void computeFov();
	void fill(Field *data);
	void rebuildBackgroundResources(QRhiCommandBuffer *cb);

protected:
	void initialize(QRhiCommandBuffer *cb) override;
	void render(QRhiCommandBuffer *cb) override;
	void releaseResources() override;
	void keyPressEvent(QKeyEvent *event) override;
	void resetResources();

private:
	QRhi *m_rhi = nullptr;
	std::unique_ptr<QRhiBuffer> m_ubuf;
	std::unique_ptr<QRhiShaderResourceBindings> m_srb;
	std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
	std::unique_ptr<QRhiTexture> m_bgTexture;
	std::unique_ptr<QRhiSampler> m_bgSampler;
	std::unique_ptr<QRhiBuffer> m_bgVbuf;
	std::unique_ptr<QRhiShaderResourceBindings> m_bgSrb;
	std::unique_ptr<QRhiGraphicsPipeline> m_bgPipeline;
	std::unique_ptr<QRhiGraphicsPipeline> m_linePipeline;
	std::unique_ptr<QRhiBuffer> m_exitsVbuf;
	std::unique_ptr<QRhiBuffer> m_markersVbuf;
	std::unique_ptr<QRhiBuffer> m_doorsVbuf;
	std::unique_ptr<QRhiBuffer> m_wireVbuf;
	QSet<QString> m_outerEdges;

	// Little helper functions as a treat
	inline bool isExitsTabSelected() const
	{
		return _selectedTabIndex == 2;
	}
	inline bool isDoorsTabSelected() const
	{
		return _selectedTabIndex == 3;
	}

	// State
	float m_xRot = 0.0f, m_yRot = 0.0f, m_zRot = 0.0f;
	float m_xTrans = 0.0f, m_yTrans = 0.0f;
	bool m_backgroundVisible = true;
	Field *m_fieldData = nullptr;
	QImage m_bgImage;
	double fovy = 70.0;
	Vertex _lineToDrawPoint1, _lineToDrawPoint2;
	int camID = 0;
	int _selectedTriangle = -1;
	int _selectedDoor = -1;
	int _selectedGate = -1;
	int _selectedTabIndex = -1;
	bool _drawLine = false;
	int m_doorsVertexCount = 0;
	int m_exitsVertexCount = 0;
	int m_markersVertexCount = 0;
	bool m_bgDirty = false;
	bool m_outerEdgesReady = false;
	int m_wireVertexCount = 0;
};
