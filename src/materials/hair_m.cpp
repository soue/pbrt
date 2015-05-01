
/*
pbrt source code Copyright(c) 1998-2012 Matt Pharr and Greg Humphreys.

This file is part of pbrt.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


// materials/hair.cpp*
#include "stdafx.h"
#include "materials/hair_m.h"
#include "spectrum.h"
#include "reflection.h"
#include "paramset.h"
#include "texture.h"
#include "textures/constant.h"

// HairMaterial Method Definitions
BSDF *HairMaterial::GetBSDF(const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgShading,
	MemoryArena &arena) const {

	// Allocate _BSDF_, possibly doing bump mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC(arena, BSDF)(dgs, dgGeom.nn);

	float ior = 1.5f;
	Spectrum r = reflect->Evaluate(dgs).Clamp();
	Spectrum t = transmit->Evaluate(dgs).Clamp();
	if (r.IsBlack() && t.IsBlack()) return bsdf;

	// Evaluate textures for _HairMaterial_ material and allocate BRDF

	Spectrum kd = Kd->Evaluate(dgs).Clamp();
	Spectrum ks = Ks->Evaluate(dgs).Clamp();
	float rough = roughness->Evaluate(dgs);
	Spectrum rr = rho_r->Evaluate(dgs).Clamp();
	Spectrum rt = rho_r->Evaluate(dgs).Clamp();

	if (!kd.IsBlack() && !ks.IsBlack()) {
		if (model == "kk") {
			if (!r.IsBlack()) bsdf->Add(BSDF_ALLOC(arena, KajiyaKayBSDF)(r * kd, r * ks, rough));
		}
		else if (model == "goldman") {
			if (!r.IsBlack()) bsdf->Add(BSDF_ALLOC(arena, GoldmanBSDF)(r * kd, r * ks, rough, rr, rt));
		}
	}

	if (!kd.IsBlack()) {
		if (!t.IsBlack()) bsdf->Add(BSDF_ALLOC(arena, BRDFToBTDF)(BSDF_ALLOC(arena, Lambertian)(t * kd)));
	}

	if (!ks.IsBlack()) {
		float rough = roughness->Evaluate(dgs);
		if (!r.IsBlack()) {
			Fresnel *fresnel = BSDF_ALLOC(arena, FresnelDielectric)(ior, 1.f);
			bsdf->Add(BSDF_ALLOC(arena, Microfacet)(r * ks, fresnel,
				BSDF_ALLOC(arena, Blinn)(1.f / rough)));
		}
		if (!t.IsBlack()) {
			Fresnel *fresnel = BSDF_ALLOC(arena, FresnelDielectric)(ior, 1.f);
			bsdf->Add(BSDF_ALLOC(arena, BRDFToBTDF)(BSDF_ALLOC(arena, Microfacet)(t * ks, fresnel,
				BSDF_ALLOC(arena, Blinn)(1.f / rough))));
		}
	}

	return bsdf;
}

HairMaterial *CreateHairMaterial(const Transform &xform,
	const TextureParams &mp) {
	Reference<Texture<Spectrum> > Kd = mp.GetSpectrumTexture("Kd", Spectrum(0.5f));
	Reference<Texture<Spectrum> > Ks = mp.GetSpectrumTexture("Ks", Spectrum(0.5f));
	Reference<Texture<float> > roughness = mp.GetFloatTexture("roughness", 0.5f);
	Reference<Texture<Spectrum> > reflect = mp.GetSpectrumTexture("reflect", Spectrum(0.5f));
	Reference<Texture<Spectrum> > transmit = mp.GetSpectrumTexture("transmit", Spectrum(0.5f));
	Reference<Texture<float> > bumpMap = mp.GetFloatTextureOrNull("bumpmap");
	string model = mp.FindString("model", "kk");
	Reference<Texture<Spectrum> > rho_r = mp.GetSpectrumTexture("rho_r", Spectrum(0.5f));
	Reference<Texture<Spectrum> > rho_t = mp.GetSpectrumTexture("rho_t", Spectrum(0.5f));
	Reference<Texture<Spectrum> > absorb = mp.GetSpectrumTexture("absorb", Spectrum(0.2f));
	Reference<Texture<float> > refr = mp.GetFloatTexture("refraction", 1.55f);
	Reference<Texture<float> > ecc = mp.GetFloatTexture("eccentricity", 0.85f);
	Reference<Texture<float> > aR = mp.GetFloatTexture("alpha", -5.0f);
	Reference<Texture<float> > aTT = new ConstantTexture<float>(-aR/2.0f);
	Reference<Texture<float> > aTRT = new ConstantTexture<float>(-3.0f*aR/2.0f);
	Reference<Texture<float> > bR = mp.GetFloatTexture("beta", -5.0f);
	Reference<Texture<float> > bTT = new ConstantTexture<float>(2.0f/bR);
	Reference<Texture<float> > bTRT = new ConstantTexture<float>(2.0f*bR);
	return new HairMaterial(Kd, Ks, roughness, reflect, transmit, rho_r, rho_t, bumpMap, model,
				refr, ecc, aR, aTT, aTRT, bR, bTT, bTRT, absorb);
}
