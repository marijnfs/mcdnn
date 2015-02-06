#ifndef __ADV_H__
#define __ADV_H__

#include "network.h"
#include "database.h"

void MakeAdvDatabase(Database &in, Database &out, Network<float> &network, float step);
void AddNAdv(Database &in, Database &out, Network<float> &network, int n, float step);


#endif
