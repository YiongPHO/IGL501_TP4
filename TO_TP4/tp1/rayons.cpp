/******************************************************************/
/*                                                                */   
/*                                                                */   
/*                                                                */   
/* Paul BODIN      -  17 146 900                                  */
/* Benjamin PRATS  -  17 107 872                                  */
/*                                                                */   
/*                                                                */   
/*                                                                */   
/*                                                                */   
/*                                                                */   
/*                                                                */   
/******************************************************************/






#include "rayons.h"
#include "couleurs.h"
#include "attr.h"
#include "ensemble.h"
#include "point.h"
#include "transfo.h"
#include "inter.h"
#include "vision.h"
#include "util.h"
#include "utilalg.h"
#include <stdio.h>
#include <math.h>




void Enregistre_pixel (int no_x, int no_y, Couleur intens, Fichier f)
// enregistre la couleur du pixel dans le fichier f
// Chacune des composantes de la couleur doit etre de 0 a 1,
// sinon elle sera ramene a une borne.
{
     
	reel r,v,b;
	char c;
 
	intens=intens*255.0;
	r=intens.rouge();
	if (r<0) r=0;
	if (r>255)r=255;
	c=(unsigned char) r;
	f.Wcarac(c);

	v=intens.vert();
	if (v<0) v=0;
	if (v>255) 
		v=255;
	c=(unsigned char) v;
	f.Wcarac(c);

	b=intens.bleu();
	if (b<0) b=0;
	if (b>255) b=255;
	c=(unsigned char) b;
	f.Wcarac(c);

}

Couleur calculIntensiteRayon(point o, vecteur dir, Objet *scene, Camera camera)
//Calcul de l'inteniste
{
	Couleur Intensite(0.0, 0.0, 0.0);

	point pt_inter;
	vecteur O;
	reel cosTheta;
	reel cosAlpha;
	vecteur L;
	vecteur H;

	Attributs a;
	Couleurs couls;

	Couleur Id, Ia, Is, Ils, Ila, Ild;
	Couleur Kd, Ka, Ks, Kr;

	vecteur ro;
	Couleur Im;

	vecteur DirectionRayon, vn, oVn;
	point OrigineRayon;
	reel distance;
	reel SL_dist;
	reel oDistance;
	Couleurs attrNonGeo;
	Couleurs oAttrNonGeo;

	reel norme;

	bool illumine = true;

	int n = 90; //Coeff Blinn 

	if (Objet_Inter(*scene, o, dir, &distance, &vn, &attrNonGeo))
	{
		
		//printf("In");
		pt_inter = o + dir*distance;
		//Calcul inteniste au point d'intersection
		Intensite = (0.0, 0.0, 0.0);
		O = -dir;

		if (vn * O < 0) //On verifie que le vecteur normal est du cote de la camera sinon on l'inverse
			vn = -vn;

		cosTheta = vn * O;

		for (int i = 0; i < camera.NbLumiere(); i++)
		{
			L = (camera.Position(i) - pt_inter);
			SL_dist = L.norme();
			L.normalise();
			H = L + O;
			H.normalise();

			//ombre
			illumine = true;
			oDistance = INFINI;
			if (Objet_Inter(*scene, pt_inter, L, &oDistance, &oVn, &oAttrNonGeo))
			{
				if (oDistance < SL_dist && oDistance > 0)
					illumine = false;
			}

			cosAlpha = H*vn;
			cosAlpha = (cosAlpha < 0 || cosTheta < 0) ? 0 : cosAlpha;

			Ild = attrNonGeo.diffus();
			Ils = camera.Diffuse(i);
			Ila = camera.Ambiante(i);

			couls = attrNonGeo;

			Kd = couls.diffus();
			Ks = couls.speculaire();
			Ka = Kd;

			Is = Ils * Ks * pow(cosAlpha, n);	//Intensite speculaire
			Id = Ild * Kd * cosTheta;	//Instensite diffuse
			Ia = Ila * Ka; //Intensite ambiante

			if (illumine)	//Sans ombre
				Intensite = Intensite + Is + Id + Ia;
			else //Avec ombre
				Intensite = Intensite + Ia;
		}

		Kr = attrNonGeo.reflechi();
		if (Kr != (0.0, 0.0, 0.0))
		{
			//printf("%f %f %f", Kr.rouge(), Kr.vert(), Kr.bleu());
			printf("In");
			ro = 2*vn*(vn * O) - O; //Rayon réfléchi
			Im = calculIntensiteRayon(pt_inter, ro, scene, camera);
			printf("%f %f %f", Im.rouge(), Im.vert(), Im.bleu());
			Intensite = Intensite + (Kr * Im);
		}
		
	}
	else
	{
		//printf("Out");
		Intensite = (0.0, 0.0, 0.0);
	}

	return Intensite;
}


booleen TraceRayons(const Camera& camera, Objet *scene, const entier& res, char fichier[])
// Genere un rayon pour chacun des pixel et calcul l'intensite de chacun
{ 
	Couleur Intensite (0.0,0.0,0.0);

	entier nb_pixel_x = res;
	entier nb_pixel_y = res;

	reel dx = 2.0 / nb_pixel_x;
	reel dy = 2.0 / nb_pixel_y;
	point o = camera.PO();
	point PmCC;
	point PmCU;
	
	vecteur dir;
	
	Transformation transfInv = Vision_Normee(camera).inverse(); // transformation de vision inverse

	//Ouverture du fichier pour y enregistrer le trace de rayon
	Fichier f;
	if ( !f.Open(fichier, "wb") ) return FAUX;

	f.Wchaine("P6\r");
	f.Wentier(res); f.Wcarac(' '); f.Wentier(res);	f.Wchaine("\r");
	f.Wentier(255);	f.Wchaine("\r");

	printf("\nDebut du trace de rayons\n");
	printf ("%4d source(s) lumineuse(s)\n", camera.NbLumiere());

	for (int no_y = 1; no_y <= nb_pixel_y; no_y++)   
	{ 
		for (int no_x = 1; no_x <= nb_pixel_x; no_x++)    
		{
			
			//Calcul de la direction dir
			PmCC = point(1.0 + (-0.5 - no_x - 1.0)*dx, 1.0 + (-0.5 - no_y - 1.0)*dy, 1) ;
			PmCU = transfInv.transforme(PmCC);
			dir = PmCU - o;
			dir.normalise();

			//Calcul intensite
			Intensite = calculIntensiteRayon(o, dir, scene, camera);				

			//On ne tient pas compte de la lumière ambiante globale

			Enregistre_pixel(no_x, no_y,Intensite, f);
		}

		//Imprime le # de ligne rendu
		if ( no_y % 15 == 0 ) printf("\n");
		printf ("%3d \n", no_y);	
	}
	printf ("\n\nFin du trace de rayons.\n");


	f.Close();
	return VRAI;
}


  
