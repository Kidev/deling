/****************************************************************************
 ** RHI Implementation
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

#include <QMatrix4x4>
#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <rhi/qshaderbaker.h>
#include <unordered_map>

struct RendererVertex {
	float position[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
	float color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	float texcoord[2]{ 0.0f, 0.0f };
};

enum RendererPrimitiveType {
	PT_POINTS,
	PT_LINES,
	PT_LINE_LOOP,
	PT_LINE_STRIP,
	PT_TRIANGLES,
	PT_TRIANGLE_STRIP,
	PT_TRIANGLE_FAN,
	PT_QUADS,
	PT_QUAD_STRIP,
	PT_POLYGON
};

class Renderer : public QObject
{
	Q_OBJECT
public:
	Renderer(QRhiWidget *widget, QRhi *rhi);
	~Renderer();

	void initialize(QRhiRenderPassDescriptor *renderPassDescriptor);

	inline bool hasError() const
	{
		return _hasError;
	}
	void clear();
	void show();
	void reset();

	void draw(QRhiCommandBuffer *cb, RendererPrimitiveType type,
	          float pointSize = 1.0f, bool clear = true);

	void setViewport(int32_t x, int32_t y, int32_t width, int32_t height);

	void bindModelMatrix(const QMatrix4x4 &matrix);
	void bindProjectionMatrix(const QMatrix4x4 &matrix);
	void bindViewMatrix(const QMatrix4x4 &matrix);

	void bindTexture(QRhiTexture *texture);
	void bindTexture(const QImage &image, bool generateMipmaps = false);

	void bufferVertex(const QVector3D &position, const QRgba64 &color,
	                  const QVector2D &texcoord);
	void clearVertices();

	void bindVertex(const RendererVertex *vertex, uint32_t count = 1);
	void bindIndex(uint32_t *index, uint32_t count = 1);

	void commitResourceUpdates(QRhiResourceUpdateBatch *batch);

private:
	bool updateBuffers();
	void createPipelines(QRhiRenderPassDescriptor *renderPassDescriptor);
	QRhiGraphicsPipeline::Topology
	rhiTopologyFromPrimitiveType(RendererPrimitiveType type);
	QRhiGraphicsPipeline *getPipelineForTopology(RendererPrimitiveType type);

	void processComplexTopology(RendererPrimitiveType type);

	QRhiWidget *mWidget;
	QRhi *mRhi;

	QRhiBuffer *mVertexBuffer;
	QRhiBuffer *mIndexBuffer;
	QRhiBuffer *mUniformBuffer;
	QRhiShaderResourceBindings *mShaderResourceBindings;

	std::unordered_map<RendererPrimitiveType, QRhiGraphicsPipeline *>
	    mPipelines;

	QRhiTexture *mDefaultTexture;
	QRhiTexture *mImageTexture;
	QRhiSampler *mDefaultSampler;

	std::vector<RendererVertex> mVertexBufferData;
	std::vector<uint32_t> mIndexBufferData;

	struct UniformData {
		QMatrix4x4 modelMatrix;
		QMatrix4x4 projectionMatrix;
		QMatrix4x4 viewMatrix;
		float pointSize;
		float padding[3]; // Align to 16 bytes
	};

	UniformData mUniformData;

	QRhiTexture *mCurrentTexture;
	QRect mViewport;
	bool _hasError;
	bool _buffersNeedUpdate;
	bool _indexBuffersNeedUpdate;
	bool _defaultTextureInitialized;
	bool _imageTextureNeedsUpdate;
};
