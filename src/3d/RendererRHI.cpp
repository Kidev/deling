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

#include "RendererRHI.h"
#include <QFile>
#include <rhi/qrhi.h>

Renderer::Renderer(QRhiWidget *widget, QRhi *rhi)
    : QObject(widget), mWidget(widget), mRhi(rhi), mVertexBuffer(nullptr),
      mIndexBuffer(nullptr), mUniformBuffer(nullptr),
      mShaderResourceBindings(nullptr), mDefaultTexture(nullptr),
      mImageTexture(nullptr), mDefaultSampler(nullptr),
      mCurrentTexture(nullptr), mViewport(0, 0, 0, 0), _hasError(false),
      _buffersNeedUpdate(true), _indexBuffersNeedUpdate(true),
      _defaultTextureInitialized(false), _imageTextureNeedsUpdate(false)
{
	mUniformData.modelMatrix.setToIdentity();
	mUniformData.projectionMatrix.setToIdentity();
	mUniformData.viewMatrix.setToIdentity();
	mUniformData.pointSize = 1.0f;
}

Renderer::~Renderer()
{
	for (auto &pair : mPipelines) {
		delete pair.second;
	}
	mPipelines.clear();

	delete mVertexBuffer;
	delete mIndexBuffer;
	delete mUniformBuffer;
	delete mShaderResourceBindings;
	delete mDefaultTexture;
	delete mImageTexture;
	delete mDefaultSampler;
}

void Renderer::initialize(QRhiRenderPassDescriptor *renderPassDescriptor)
{
	mVertexBuffer =
	    mRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, 1024);
	if (!mVertexBuffer->create()) {
		qWarning() << "Failed to create vertex buffer";
		_hasError = true;
		return;
	}

	mIndexBuffer =
	    mRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::IndexBuffer, 1024);
	if (!mIndexBuffer->create()) {
		qWarning() << "Failed to create index buffer";
		_hasError = true;
		return;
	}

	mUniformBuffer = mRhi->newBuffer(
	    QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UniformData));
	if (!mUniformBuffer->create()) {
		qWarning() << "Failed to create uniform buffer";
		_hasError = true;
		return;
	}

	mShaderResourceBindings = mRhi->newShaderResourceBindings();

	mDefaultTexture = mRhi->newTexture(QRhiTexture::RGBA8, QSize(1, 1));
	mDefaultTexture->create();

	mDefaultSampler = mRhi->newSampler(
	    QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
	    QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
	mDefaultSampler->create();

	mShaderResourceBindings->setBindings(
	    { QRhiShaderResourceBinding::uniformBuffer(
	          0,
	          QRhiShaderResourceBinding::VertexStage
	              | QRhiShaderResourceBinding::FragmentStage,
	          mUniformBuffer),
	      QRhiShaderResourceBinding::sampledTexture(
	          1, QRhiShaderResourceBinding::FragmentStage, mDefaultTexture,
	          mDefaultSampler) });

	if (!mShaderResourceBindings->create()) {
		qWarning() << "Failed to create shader resource bindings";
		_hasError = true;
		return;
	}

	createPipelines(renderPassDescriptor);
}

void Renderer::createPipelines(QRhiRenderPassDescriptor *renderPassDescriptor)
{
	QShader vertexShader;
	QFile vFile(":/shaders/prebuilt/color.vert.qsb");
	if (vFile.open(QIODevice::ReadOnly)) {
		vertexShader = QShader::fromSerialized(vFile.readAll());
	}
	if (!vertexShader.isValid()) {
		qWarning() << "Failed to load vertex shader";
		_hasError = true;
		return;
	}

	QShader fragmentShader;
	QFile fFile(":/shaders/prebuilt/color.frag.qsb");
	if (fFile.open(QIODevice::ReadOnly)) {
		fragmentShader = QShader::fromSerialized(fFile.readAll());
	}
	if (!fragmentShader.isValid()) {
		qWarning() << "Failed to load fragment shader";
		_hasError = true;
		return;
	}

	QRhiVertexInputLayout inputLayout;
	inputLayout.setBindings({ { sizeof(RendererVertex) } });
	inputLayout.setAttributes({
	    { 0, 0, QRhiVertexInputAttribute::Float4, 0 }, // position
	    { 0, 1, QRhiVertexInputAttribute::Float4, 4 * sizeof(float) }, // color
	    { 0, 2, QRhiVertexInputAttribute::Float2,
	      8 * sizeof(float) } // texcoord
	});

	const std::vector<RendererPrimitiveType> topologies = {
		PT_POINTS, PT_LINES, PT_LINE_STRIP, PT_TRIANGLES, PT_TRIANGLE_STRIP
	};

	for (RendererPrimitiveType topology : topologies) {
		QRhiGraphicsPipeline *pipeline = mRhi->newGraphicsPipeline();

		pipeline->setShaderStages(
		    { { QRhiShaderStage::Vertex, vertexShader },
		      { QRhiShaderStage::Fragment, fragmentShader } });

		pipeline->setTopology(rhiTopologyFromPrimitiveType(topology));
		pipeline->setVertexInputLayout(inputLayout);
		pipeline->setShaderResourceBindings(mShaderResourceBindings);
		pipeline->setRenderPassDescriptor(renderPassDescriptor);

		// Enable blending
		QRhiGraphicsPipeline::TargetBlend blend;
		blend.enable = true;
		blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
		blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
		blend.srcAlpha = QRhiGraphicsPipeline::One;
		blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
		pipeline->setTargetBlends({ blend });

		// Enable depth testing
		pipeline->setDepthTest(true);
		pipeline->setDepthWrite(true);
		pipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);

		if (!pipeline->create()) {
			qWarning() << "Failed to create graphics pipeline for topology"
			           << topology;
			_hasError = true;
			delete pipeline;
			return;
		}

		mPipelines[topology] = pipeline;
	}
}

void Renderer::clear()
{
	// RHI clearing handled by command buffer
}

void Renderer::show()
{
	mWidget->update();
}

void Renderer::reset()
{
	mUniformData.projectionMatrix.setToIdentity();
	mUniformData.viewMatrix.setToIdentity();
	mUniformData.modelMatrix.setToIdentity();
}

void Renderer::setViewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
	mViewport = QRect(x, y, width, height);
}

bool Renderer::updateBuffers()
{
	bool needsUpdate = false;

	// Update vertex buffer
	if (!mVertexBufferData.empty()) {
		size_t requiredSize = mVertexBufferData.size() * sizeof(RendererVertex);
		if (static_cast<size_t>(mVertexBuffer->size()) < requiredSize) {
			delete mVertexBuffer;
			mVertexBuffer = mRhi->newBuffer(
			    QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
			    static_cast<quint32>(requiredSize * 2)); // Over-allocate
			if (!mVertexBuffer->create()) {
				qWarning() << "Failed to recreate vertex buffer";
				return false;
			}
			needsUpdate = true;
		}
		if (_buffersNeedUpdate || needsUpdate) {
			needsUpdate = true;
		}
	}

	// Update index buffer
	if (!mIndexBufferData.empty()) {
		size_t requiredSize = mIndexBufferData.size() * sizeof(uint32_t);
		if (static_cast<size_t>(mIndexBuffer->size()) < requiredSize) {
			delete mIndexBuffer;
			mIndexBuffer = mRhi->newBuffer(
			    QRhiBuffer::Dynamic, QRhiBuffer::IndexBuffer,
			    static_cast<quint32>(requiredSize * 2)); // Over-allocate
			if (!mIndexBuffer->create()) {
				qWarning() << "Failed to recreate index buffer";
				return false;
			}
			needsUpdate = true;
		}
		if (_indexBuffersNeedUpdate || needsUpdate) {
			needsUpdate = true;
		}
	}

	return true;
}

QRhiGraphicsPipeline::Topology
Renderer::rhiTopologyFromPrimitiveType(RendererPrimitiveType type)
{
	switch (type) {
	case PT_POINTS:
		return QRhiGraphicsPipeline::Points;
	case PT_LINES:
	case PT_LINE_LOOP:
	case PT_LINE_STRIP:
		return QRhiGraphicsPipeline::Lines;
	case PT_TRIANGLES:
	case PT_TRIANGLE_STRIP:
	case PT_TRIANGLE_FAN:
	case PT_QUADS:
	case PT_QUAD_STRIP:
	case PT_POLYGON:
	default:
		return QRhiGraphicsPipeline::Triangles;
	}
}

QRhiGraphicsPipeline *
Renderer::getPipelineForTopology(RendererPrimitiveType type)
{
	QRhiGraphicsPipeline::Topology rhiTopology =
	    rhiTopologyFromPrimitiveType(type);

	RendererPrimitiveType pipelineType;
	switch (rhiTopology) {
	case QRhiGraphicsPipeline::Points:
		pipelineType = PT_POINTS;
		break;
	case QRhiGraphicsPipeline::Lines:
		pipelineType = PT_LINES;
		break;
	case QRhiGraphicsPipeline::Triangles:
	default:
		pipelineType = PT_TRIANGLES;
		break;
	}

	auto it = mPipelines.find(pipelineType);
	return (it != mPipelines.end()) ? it->second : mPipelines[PT_TRIANGLES];
}

void Renderer::processComplexTopology(RendererPrimitiveType type)
{
	switch (type) {
	case PT_LINE_LOOP:
		// Convert line loop to line strip by adding closing line
		if (!mVertexBufferData.empty()) {
			if (mIndexBufferData.empty()) {
				for (size_t i = 0; i < mVertexBufferData.size(); ++i) {
					mIndexBufferData.push_back(static_cast<uint32_t>(i));
				}
			}
			// Add closing line
			if (!mIndexBufferData.empty()) {
				mIndexBufferData.push_back(mIndexBufferData[0]);
			}
			_indexBuffersNeedUpdate = true;
		}
		break;

	case PT_TRIANGLE_FAN:
		// Convert triangle fan to triangles
		if (mVertexBufferData.size() >= 3) {
			mIndexBufferData.clear();
			for (size_t i = 1; i < mVertexBufferData.size() - 1; ++i) {
				mIndexBufferData.push_back(0);
				mIndexBufferData.push_back(static_cast<uint32_t>(i));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 1));
			}
			_indexBuffersNeedUpdate = true;
		}
		break;

	case PT_QUADS:
		// Convert quads to triangles
		if (mVertexBufferData.size() % 4 == 0) {
			mIndexBufferData.clear();
			for (size_t i = 0; i < mVertexBufferData.size(); i += 4) {
				// First triangle
				mIndexBufferData.push_back(static_cast<uint32_t>(i));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 1));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 2));
				// Second triangle
				mIndexBufferData.push_back(static_cast<uint32_t>(i));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 2));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 3));
			}
			_indexBuffersNeedUpdate = true;
		}
		break;

	case PT_QUAD_STRIP:
		// Convert quad strip to triangle strip
		if (mVertexBufferData.size() >= 4
		    && mVertexBufferData.size() % 2 == 0) {
			mIndexBufferData.clear();
			for (size_t i = 0; i < mVertexBufferData.size() - 2; i += 2) {
				// First triangle
				mIndexBufferData.push_back(static_cast<uint32_t>(i));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 1));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 2));
				// Second triangle
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 1));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 3));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 2));
			}
			_indexBuffersNeedUpdate = true;
		}
		break;

	case PT_POLYGON:
		// Convert polygon to triangle fan
		if (mVertexBufferData.size() >= 3) {
			mIndexBufferData.clear();
			for (size_t i = 1; i < mVertexBufferData.size() - 1; ++i) {
				mIndexBufferData.push_back(0);
				mIndexBufferData.push_back(static_cast<uint32_t>(i));
				mIndexBufferData.push_back(static_cast<uint32_t>(i + 1));
			}
			_indexBuffersNeedUpdate = true;
		}
		break;

	default:
		// Generate simple indices if none exist
		if (mIndexBufferData.empty() && !mVertexBufferData.empty()) {
			for (size_t i = 0; i < mVertexBufferData.size(); ++i) {
				mIndexBufferData.push_back(static_cast<uint32_t>(i));
			}
			_indexBuffersNeedUpdate = true;
		}
		break;
	}
}

void Renderer::draw(QRhiCommandBuffer *cb, RendererPrimitiveType type,
                    float pointSize, bool clear)
{
	if (mVertexBufferData.empty() || !updateBuffers()) {
		return;
	}

	processComplexTopology(type);

	mUniformData.pointSize = pointSize;

	QRhiResourceUpdateBatch *batch = mRhi->nextResourceUpdateBatch();

	if (!_defaultTextureInitialized) {
		QImage whitePixel(1, 1, QImage::Format_RGBA8888);
		whitePixel.fill(Qt::white);
		batch->uploadTexture(mDefaultTexture, whitePixel);
		_defaultTextureInitialized = true;
	}

	if (_imageTextureNeedsUpdate && mImageTexture) {
		_imageTextureNeedsUpdate = false;
	}

	if (_buffersNeedUpdate || !mVertexBufferData.empty()) {
		batch->updateDynamicBuffer(
		    mVertexBuffer, 0, mVertexBufferData.size() * sizeof(RendererVertex),
		    mVertexBufferData.data());
		_buffersNeedUpdate = false;
	}

	if (_indexBuffersNeedUpdate && !mIndexBufferData.empty()) {
		batch->updateDynamicBuffer(mIndexBuffer, 0,
		                           mIndexBufferData.size() * sizeof(uint32_t),
		                           mIndexBufferData.data());
		_indexBuffersNeedUpdate = false;
	}

	batch->updateDynamicBuffer(mUniformBuffer, 0, sizeof(UniformData),
	                           &mUniformData);

	cb->resourceUpdate(batch);

	QRhiGraphicsPipeline *pipeline = getPipelineForTopology(type);
	if (!pipeline) {
		qWarning() << "No pipeline available for topology" << type;
		return;
	}

	cb->setGraphicsPipeline(pipeline);
	cb->setShaderResources(mShaderResourceBindings);

	if (!mViewport.isEmpty()) {
		cb->setViewport(QRhiViewport(mViewport.x(), mViewport.y(),
		                             mViewport.width(), mViewport.height()));
	}

	const QRhiCommandBuffer::VertexInput vbuf(mVertexBuffer, 0);
	cb->setVertexInput(0, 1, &vbuf);

	if (!mIndexBufferData.empty()) {
		cb->setVertexInput(0, 1, &vbuf, mIndexBuffer, 0,
		                   QRhiCommandBuffer::IndexUInt32);
		cb->drawIndexed(mIndexBufferData.size());
	} else {
		cb->draw(mVertexBufferData.size());
	}

	if (clear) {
		mVertexBufferData.clear();
		mIndexBufferData.clear();
		_buffersNeedUpdate = true;
		_indexBuffersNeedUpdate = true;
	}
}

void Renderer::bindModelMatrix(const QMatrix4x4 &matrix)
{
	mUniformData.modelMatrix = matrix;
}

void Renderer::bindProjectionMatrix(const QMatrix4x4 &matrix)
{
	mUniformData.projectionMatrix = mRhi->clipSpaceCorrMatrix() * matrix;
}

void Renderer::bindViewMatrix(const QMatrix4x4 &matrix)
{
	mUniformData.viewMatrix = matrix;
}

void Renderer::bindTexture(QRhiTexture *texture)
{
	if (texture != mCurrentTexture) {
		mCurrentTexture = texture;

		mShaderResourceBindings->setBindings(
		    { QRhiShaderResourceBinding::uniformBuffer(
		          0,
		          QRhiShaderResourceBinding::VertexStage
		              | QRhiShaderResourceBinding::FragmentStage,
		          mUniformBuffer),
		      QRhiShaderResourceBinding::sampledTexture(
		          1, QRhiShaderResourceBinding::FragmentStage,
		          texture ? texture : mDefaultTexture, mDefaultSampler) });

		mShaderResourceBindings->create();
	}
}

void Renderer::bindTexture(const QImage &image, bool generateMipmaps)
{
	Q_UNUSED(generateMipmaps); // RHI handles mipmaps differently

	if (mImageTexture) {
		delete mImageTexture;
	}

	mImageTexture = mRhi->newTexture(QRhiTexture::RGBA8, image.size());
	if (!mImageTexture->create()) {
		qWarning() << "Failed to create image texture";
		return;
	}

	QRhiResourceUpdateBatch *batch = mRhi->nextResourceUpdateBatch();
	batch->uploadTexture(mImageTexture, image);

	bindTexture(mImageTexture);
	_imageTextureNeedsUpdate = true;
}

void Renderer::bufferVertex(const QVector3D &position, const QRgba64 &color,
                            const QVector2D &texcoord)
{
	RendererVertex vertex;
	vertex.position[0] = position.x();
	vertex.position[1] = position.y();
	vertex.position[2] = position.z();
	vertex.position[3] = 1.0f;

	vertex.color[0] = color.red() / 65535.0f;
	vertex.color[1] = color.green() / 65535.0f;
	vertex.color[2] = color.blue() / 65535.0f;
	vertex.color[3] = color.alpha() / 65535.0f;

	vertex.texcoord[0] = texcoord.x();
	vertex.texcoord[1] = texcoord.y();

	mVertexBufferData.push_back(vertex);
	_buffersNeedUpdate = true;
}

void Renderer::bindVertex(const RendererVertex *vertex, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		mVertexBufferData.push_back(vertex[i]);
	}
	_buffersNeedUpdate = count > 0;
}

void Renderer::bindIndex(uint32_t *index, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		mIndexBufferData.push_back(index[i]);
	}
	_indexBuffersNeedUpdate = count > 0;
}

void Renderer::clearVertices()
{
	mVertexBufferData.clear();
	mIndexBufferData.clear();
	_buffersNeedUpdate = true;
	_indexBuffersNeedUpdate = true;
}

void Renderer::commitResourceUpdates(QRhiResourceUpdateBatch *batch)
{
	Q_UNUSED(batch); // The batch will be committed in the draw() method
	if (_imageTextureNeedsUpdate && mImageTexture) {
		_imageTextureNeedsUpdate = false;
	}
}
