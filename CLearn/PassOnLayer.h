#pragma once
#include "DiscarnateLayer.h"

#ifndef CNET_PASSON
#define CNET_PASSON
/* Applies an element-wise function of choice.
*/
class PassOnLayer : public DiscarnateLayer{

public:
	PassOnLayer(size_t NOUT, size_t NIN, actfunc_t type);
	PassOnLayer(size_t NOUT, actfunc_t type, CNetLayer& lower);
	PassOnLayer(actfunc_t type, CNetLayer& lower);

	~PassOnLayer();
	layer_t whoAmI() const;
	// forProp
	void forProp(MAT& in, bool saveActivation, bool recursive); // recursive
	void backPropDelta(MAT& delta, bool recursive); // recursive

private:
	void init();
	void saveToFile(ostream& os) const;
	void loadFromFile(ifstream& in);
};

#endif