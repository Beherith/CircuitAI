#include "role.as"


namespace Init {

void Init(dictionary@ categories)
{
	aiLog("AngelScript Rules!");

	categories["air"]   = "VTOL NOTSUB";
	categories["land"]  = "SURFACE NOTSUB";
	categories["water"] = "UNDERWATER NOTHOVER";
	categories["bad"]   = "TERRAFORM STUPIDTARGET MINE";
	categories["good"]  = "TURRET FLOAT";
}

}
