#include "stdafx.h"
#include "ConvolutionalLayer.h"

ConvolutionalLayer::ConvolutionalLayer(size_t _NOUTX, size_t _NOUTY, size_t _NINX, size_t _NINY, size_t _kernelX, size_t _kernelY, uint32_t _stride, actfunc_t type)
	: NOUTX(_NOUTX), NOUTY(_NOUTY), NINX(_NINX), NINY(_NINY), kernelX(_kernelX), kernelY(_kernelY), stride(_stride), PhysicalLayer(_NOUTX*_NOUTY, _NINX*_NINY, type){
	// the layer matrix will act as convolutional kernel
	init();
}

ConvolutionalLayer::ConvolutionalLayer(size_t _NOUTX, size_t _NOUTY, size_t _NINX, size_t _NINY, size_t _kernelX, size_t _kernelY, uint32_t _stride , actfunc_t type, CNetLayer& const lower)
: NOUTX(_NOUTX), NOUTY(_NOUTY), NINX(_NINX), NINY(_NINY), kernelX(_kernelX), kernelY(_kernelY), stride(_stride), PhysicalLayer(_NOUTX*_NOUTY,  type, lower) {
	init();
	assertGeometry();
}
// second most convenient constructor
ConvolutionalLayer::ConvolutionalLayer(size_t NOUTXY, size_t NINXY, size_t kernelXY, uint32_t _stride, actfunc_t type) 
	: NOUTX(NOUTXY), NOUTY(NOUTXY), NINX(NINXY), NINY(NINXY), kernelX(kernelXY), kernelY(kernelXY), stride(_stride), PhysicalLayer(NOUTXY*NOUTXY, NINXY*NINXY, type) {
	init();
	assertGeometry();
}

// most convenient constructor
ConvolutionalLayer::ConvolutionalLayer(size_t NOUTXY, size_t kernelXY, uint32_t _stride, actfunc_t type, CNetLayer& const lower)
	: NOUTX(NOUTXY), NOUTY(NOUTXY), NINX(sqrt(lower.getNOUT())), NINY(sqrt(lower.getNOUT())), kernelX(kernelXY), kernelY(kernelXY), stride(_stride), PhysicalLayer(NOUTXY*NOUTXY, type, lower) {
	init();
	assertGeometry();
}
// destructor
ConvolutionalLayer::~ConvolutionalLayer() {
	layer.resize(0, 0);
}

layer_t ConvolutionalLayer::whoAmI() const {
	return layer_t::convolutional;
}
void ConvolutionalLayer::assertGeometry() {
	assert(NOUTX*NOUTY == getNOUT());
	assert(NINX*NINY == getNIN());
	assert((stride*NOUTX - stride - NINX + kernelX) % 2 == 0);
	assert((stride*NOUTY - stride - NINY + kernelY) % 2 == 0);
}
void ConvolutionalLayer::init() {
	layer = MAT(kernelY, kernelX);
	gradient = MAT(kernelY, kernelX);
	vel = MAT(kernelY, kernelX);
	actSave = MAT(NOUT,1);
	deltaSave = MAT(NOUT,1);
	prevStep = MAT(layer.rows(), layer.cols());

	prevStep.setConstant(0);
	layer.setRandom();
	layer += MAT::Constant(layer.cols(), layer.rows(), 1);
	layer *= 1 / layer.size();

	gradient.setConstant(0);
	vel.setConstant(0);
	actSave.setConstant(0);
	deltaSave.setConstant(0);
}

void ConvolutionalLayer::forProp(MAT& inBelow, learnPars& const pars, bool training) {
	inBelow.resize(NINY, NINX);
	MAT convoluted = conv(inBelow, layer, stride, padSize(NOUTY, NINY, kernelY, stride), padSize(NOUTX, NINX, kernelX, stride)); // square convolution
	convoluted.resize(NOUTX*NOUTY, 1);
	if (training) {
		actSave = convoluted;
		if (hierarchy != hierarchy_t::output) {
			inBelow = actSave.unaryExpr(act);
			above->forProp(inBelow, pars, true);
		} else {
			inBelow = actSave;
		}
	} else {
		if (hierarchy != hierarchy_t::output) {
			inBelow = convoluted.unaryExpr(act);
			above->forProp(inBelow, pars, false);
		}
		else {
			inBelow = convoluted;
		}
	}
}
// backprop
void ConvolutionalLayer::backPropDelta(MAT& const deltaAbove) {
	deltaSave = deltaAbove;

	if (hierarchy != hierarchy_t::input) { // ... this is not an input layer.
		deltaSave.resize(NOUTY, NOUTX); // keep track of this!!!
		MAT convoluted = antiConv(deltaSave, layer, stride, padSize(NOUTY, NINY, kernelY, stride), padSize(NOUTX, NINX, kernelX, stride));
		//MAT convoluted = conv(deltaSave, layer, stride, padSize(NINY, NOUTY, kernelY, stride), padSize(NINX, NOUTX, kernelX, stride));
		convoluted.resize(NIN, 1);
		convoluted = convoluted.cwiseProduct(below->getDACT()); // multiply with h'(aj)
		deltaSave.resize(NOUT, 1); // resize back
		below->backPropDelta(convoluted); // cascade...
	}
}
// grad
MAT ConvolutionalLayer::grad(MAT& const input) {
	deltaSave.resize(NOUTY, NOUTX);
	if (hierarchy == hierarchy_t::input) {
		input.resize(NINY, NINX);
		//MAT convoluted = conv(deltaSave, input, stride, padSize(kernelY,  NOUTY, NINY, stride), padSize(kernelX,  NOUTX, NINX, stride)).reverse();
		MAT convoluted = convGrad(deltaSave, input, stride, kernelY, kernelX, padSize(NOUTY, NINY, kernelY, stride), padSize(NOUTX, NINX, kernelX, stride));
		
		//MAT convoluted = deltaActConv(deltaSave, input, kernelY, kernelX, stride, padSize(NOUTY, NINY, kernelY, stride), padSize(NOUTX, NINX, kernelX, stride));
		deltaSave.resize(NOUTY*NOUTX, 1);
		input.resize(NIN, 1);
		return convoluted;
	} else {
		MAT fromBelow = below->getACT();
		fromBelow.resize(NINY, NINX);
		//MAT convoluted = conv( deltaSave, fromBelow, stride, padSize(kernelY,NOUTY, NINY, stride), padSize(kernelX,  NOUTX, NINX, stride)).reverse();
		MAT convoluted = convGrad( deltaSave, fromBelow, stride, kernelY, kernelX, padSize(NOUTY, NINY, kernelY, stride), padSize(NOUTX, NINX, kernelX, stride));
		//MAT convoluted = deltaActConv(deltaSave, fromBelow, kernelY, kernelX, stride, padSize(NOUTY, NINY, kernelY, stride), padSize(NOUTX, NINX, kernelX, stride));
		deltaSave.resize(NOUT, 1);
		return convoluted;
	}
}
void ConvolutionalLayer::saveToFile(ostream& os) const {
	os << NOUTY << "\t" << NOUTX << "\t" << NINY << "\t" << NINX << "\t" << kernelY << "\t" << kernelX << "\t"  << endl;
	os << layer;
}
// first line has been read already
void ConvolutionalLayer::loadFromFile(ifstream& in) {
	in >> NOUTY;
	in >> NOUTX;
	in >> NINY;
	in >> NINX;
	in >> kernelY;
	in >> kernelX;

	for (size_t i = 0; i < kernelY; i++) {
		for (size_t j = 0; j < kernelX; j++) {
			in >> layer(i, j);
		}
	}
}