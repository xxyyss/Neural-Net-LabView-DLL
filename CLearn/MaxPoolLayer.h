#pragma once
#include "DiscarnateLayer.h"
#ifndef CNET_CMAXPOOL
#define CNET_CMAXPOOL

/* Implements a max-pool operation.
* 
*/
class MaxPoolLayer : public DiscarnateLayer{

public:
	MaxPoolLayer(size_t NINXY, size_t maxOver, size_t channels);
	MaxPoolLayer(size_t maxOver, size_t channels, CNetLayer& lower);
	MaxPoolLayer(size_t channels, CNetLayer& lower);

	~MaxPoolLayer();
	layer_t whoAmI() const;
	// forProp
	void forProp(MAT& in, bool saveActivation, bool recursive); // recursive
	void backPropDelta(MAT& delta, bool recursive); // recursive
	
	inline size_t getMaxOverX() { return maxOverX; }
	inline size_t getMaxOverY() { return maxOverY; }
	inline size_t getNINX() { return NINX; };
	inline size_t getNINY() { return NINY; };
	inline size_t getNOUTX() { return NOUTX; };
	inline size_t getNOUTY() { return NOUTY; };


private:
	size_t maxOverX;
	size_t maxOverY;
	size_t NINX;
	size_t NINY;
	size_t NOUTX;
	size_t NOUTY;
	MATINDEX indexX;
	MATINDEX indexY;
	size_t channels;

	MAT maxPool(const MAT& in,  bool saveIndices);
	void assertGeometry();
	void saveToFile(ostream& os) const;
	void loadFromFile(ifstream& in);
	void init();
};

#endif