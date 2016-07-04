/* =============================================================================
**  This file is part of the mmg software package for the tetrahedral
**  mesh modification.
**  Copyright (c) Bx INP/Inria/UBordeaux/UPMC, 2004- .
**
**  mmg is free software: you can redistribute it and/or modify it
**  under the terms of the GNU Lesser General Public License as published
**  by the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  mmg is distributed in the hope that it will be useful, but WITHOUT
**  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
**  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
**  License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License and of the GNU General Public License along with mmg (in
**  files COPYING.LESSER and COPYING). If not, see
**  <http://www.gnu.org/licenses/>. Please read their terms carefully and
**  use this copy of the mmg distribution only if you accept them.
** =============================================================================
*/
/**
 * \file mmg2d/split_2d.c
 * \brief Functions for splitting.
 * \author Charles Dapogny (UPMC)
 * \author Cécile Dobrzynski (Bx INP/Inria/UBordeaux)
 * \author Pascal Frey (UPMC)
 * \author Algiane Froehly (Inria/UBordeaux)
 * \version 5
 * \copyright GNU Lesser General Public License.
 */

#include "mmg2d.h"

/* Check whether splitting of edge i in tria k is possible and return the newly created point;
   possibly perform a dichotomy to find the latest valid position for the point */
int _MMG2_chkspl(MMG5_pMesh mesh,MMG5_pSol met,int k,char i) {
  MMG5_pTria           pt,pt1,pt0;
  MMG5_pPoint          p1,p2,ppt;
  double               mid[2],o[2],no[2],calnew,caltmp,tp,to,t;
  int                  ip,jel,*adja,it,maxit,npinit;
  char                 i1,i2,j,j1,j2,ier,isv;

  npinit = mesh->np;

  pt  = &mesh->tria[k];
  pt0 = &mesh->tria[0];
  i1  = _MMG5_inxt2[i];
  i2  = _MMG5_iprv2[i];

  p1 = &mesh->point[pt->v[i1]];
  p2 = &mesh->point[pt->v[i2]];

  adja = &mesh->adja[3*(k-1)+1];
  jel  = adja[i] / 3;
  j    = adja[i] % 3;
  j1   = _MMG5_inxt2[j];
  j2   = _MMG5_iprv2[j];

  /* Midpoint of edge i */
  mid[0] = 0.5*(p1->c[0]+p2->c[0]);
  mid[1] = 0.5*(p1->c[1]+p2->c[1]);

  /* If the splitted edge is not geometric, the new point is simply its midpoint */
  if ( !MG_EDG(pt->tag[i]) ) {
    ip = _MMG2D_newPt(mesh,mid,0);
    if ( !ip ) {
      /* reallocation of point table */
      _MMG2D_POINT_REALLOC(mesh,met,ip,mesh->gap,
                           printf("  ## Error: unable to allocate a new point.\n");
                           _MMG5_INCREASE_MEM_MESSAGE();
                           do {
                             _MMG2D_delPt(mesh,mesh->np);
                           } while ( mesh->np>npinit );
                           return(-1)
                           ,mid,pt->tag[i]);

    }

    ppt = &mesh->point[ip];
    if ( pt->tag[i] ) ppt->tag = pt->tag[i];
    if ( pt->edg[i] ) ppt->ref = pt->edg[i];

    /* Check quality of the four new elements: to do ANISOTROPIC */
    calnew = DBL_MAX;
    memcpy(pt0,pt,sizeof(MMG5_Tria));
    pt0->v[i2] = ip;
    caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
    calnew = MG_MIN(calnew,caltmp);

    pt0->v[i1] = ip; pt0->v[i2] = pt->v[i2];
    caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
    calnew = MG_MIN(calnew,caltmp);

    if ( jel ) {
      pt1 = &mesh->tria[jel];
      memcpy(pt0,pt1,sizeof(MMG5_Tria));
      pt0->v[j1] = ip;
      caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
      calnew = MG_MIN(calnew,caltmp);

      pt0->v[j1] = pt1->v[j1] ; pt0->v[j2] = ip;
      caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
      calnew = MG_MIN(calnew,caltmp);
    }

    /* Delete point and abort splitting if one of the created triangles is nearly degenerate */
    if ( calnew < _MMG5_EPSD ) {
      _MMG2D_delPt(mesh,ip);
      return(0);
    }
  }
  /* Otherwise, the new point is inserted on the underlying curve to the edge;
     a dichotomy is applied to find the largest distance to the edge that yields an admissible configuration */
  else {
    ier = _MMG2_bezierCurv(mesh,k,i,0.5,o,no);
    ip  = _MMG2D_newPt(mesh,o,pt->tag[i]);
    if ( !ip ) {
      /* reallocation of point table */
      _MMG2D_POINT_REALLOC(mesh,met,ip,mesh->gap,
                           printf("  ## Error: unable to allocate a new point.\n");
                           _MMG5_INCREASE_MEM_MESSAGE();
                           do {
                             _MMG2D_delPt(mesh,mesh->np);
                           } while ( mesh->np>npinit );
                           return(-1)
                           ,o,pt->tag[i]);
    }

    ppt = &mesh->point[ip];
    if ( pt->tag[i] ) ppt->tag = pt->tag[i];
    if ( pt->edg[i] ) ppt->ref = pt->edg[i];

    ppt->n[0] = no[0];
    ppt->n[1] = no[1];

    isv   = 0;
    it    = 0;
    maxit = 5;
    tp    = 1.0;
    t     = 1.0;
    to    = 0.0;

    do {
      ppt->c[0] = mid[0] + t*(o[0] - mid[0]);
      ppt->c[1] = mid[1] + t*(o[1] - mid[1]);

      /* Check quality of the four new elements: to do ANISOTROPIC */
      calnew = DBL_MAX;
      memcpy(pt0,pt,sizeof(MMG5_Tria));
      pt0->v[i2] = ip;
      caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
      calnew = MG_MIN(calnew,caltmp);

      pt0->v[i1] = ip; pt0->v[i2] = pt->v[i2];
      caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
      calnew = MG_MIN(calnew,caltmp);

      if ( jel ) {
        pt1 = &mesh->tria[jel];
        memcpy(pt0,pt1,sizeof(MMG5_Tria));
        pt0->v[j1] = ip;
        caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
        calnew = MG_MIN(calnew,caltmp);

        pt0->v[j1] = pt1->v[j1] ; pt0->v[j2] = ip;
        caltmp = ALPHAD*caltri_iso(mesh,NULL,pt0);
        calnew = MG_MIN(calnew,caltmp);
      }

      ier = ( calnew > _MMG5_EPSD );
      if ( ier ) {
        isv = 1;
        if ( t == tp ) break;
        else
          to = t;
      }
      else
        tp = t;

      /* If no admissible position has been found, do the last iteration with the midpoint m */
      if ( (it == maxit-2) && !isv )
        t = 0.0;
      else
        t = 0.5*(to+tp);
    }
    while ( ++it < maxit );

    /* No satisfying position has been found */
    if ( !isv ) {
      _MMG2D_delPt(mesh,ip);
      return(0);
    }
  }

  /* Interpolate metric at ip, if any */
  if ( met->m )
    _MMG2_intmet_iso(mesh,met,k,i,ip,0.5);

  return(ip);
}

/* Effective splitting of edge i in tria k: point ip is introduced and the adjacency structure in the mesh is preserved */
int _MMG2_split1b(MMG5_pMesh mesh,int k,char i,int ip) {
  MMG5_pTria         pt,pt1;
  int                *adja,iel,jel,kel,mel;
  char               i1,i2,m,j,j1,j2;

  iel = _MMG2D_newElt(mesh);
  if ( !iel ) {
    _MMG2D_TRIA_REALLOC(mesh,iel,mesh->gap,
                        printf("  ## Error: unable to allocate a new element.\n");
                        _MMG5_INCREASE_MEM_MESSAGE();
                        printf("  Exit program.\n");
                        exit(EXIT_FAILURE)
      );
  }

  pt = &mesh->tria[k];
  pt->flag = 0;
  pt->base = mesh->base;

  i1 = _MMG5_inxt2[i];
  i2 = _MMG5_iprv2[i];

  adja = &mesh->adja[3*(k-1)+1];
  jel  = adja[i] / 3;
  j    = adja[i] % 3;

  pt1 = &mesh->tria[iel];
  memcpy(pt1,pt,sizeof(MMG5_Tria));
  memcpy(&mesh->adja[3*(iel-1)+1],&mesh->adja[3*(k-1)+1],3*sizeof(int));

  /* Update both triangles */
  pt->v[i2]  = ip;
  pt1->v[i1] = ip;

  pt->tag[i1] = MG_NOTAG;
  pt->edg[i1] = 0;

  pt1->tag[i2] = MG_NOTAG;
  pt1->edg[i2] = 0;

  /* Update adjacencies */
  mel = adja[i1] / 3;
  m   = adja[i1] % 3;
  mesh->adja[3*(k-1)+1+i1] = 3*iel+i2;
  mesh->adja[3*(iel-1)+1+i2] = 3*k+i1;
  if ( mel )
    mesh->adja[3*(mel-1)+1+m] = 3*iel+i1;

  if ( jel ) {
    kel = _MMG2D_newElt(mesh);
    if ( !kel ) {
      _MMG2D_TRIA_REALLOC(mesh,kel,mesh->gap,
                          printf("  ## Error: unable to allocate a new element.\n");
                          _MMG5_INCREASE_MEM_MESSAGE();
                          printf("  Exit program.\n");
                          exit(EXIT_FAILURE)
        );
    }

    pt  = &mesh->tria[jel];
    pt1 = &mesh->tria[kel];
    j1 = _MMG5_inxt2[j];
    j2 = _MMG5_iprv2[j];

    pt->flag = 0;
    pt->base = mesh->base;

    memcpy(pt1,pt,sizeof(MMG5_Tria));
    memcpy(&mesh->adja[3*(kel-1)+1],&mesh->adja[3*(jel-1)+1],3*sizeof(int));

    /* Update triangles */
    pt->v[j1]    = ip;
    pt1->v[j2]   = ip;
    pt->tag[j2]  = MG_NOTAG;
    pt->edg[j2]  = 0;
    pt1->tag[j1] = MG_NOTAG;
    pt1->edg[j1] = 0;

    /* Update adjacencies */
    adja = &mesh->adja[3*(jel-1)+1];
    mel  = adja[j2] / 3;
    m    = adja[j2] % 3;
    mesh->adja[3*(jel-1)+1+j2] = 3*kel+j1;
    mesh->adja[3*(kel-1)+1+j1] = 3*jel+j2;
    if ( mel )
      mesh->adja[3*(mel-1)+1+m] = 3*kel+j2;

    mesh->adja[3*(iel-1)+1+i] = 3*kel+j;
    mesh->adja[3*(kel-1)+1+j] = 3*iel+i;
  }

  return(1);
}

/* Simulate the split of one edge in triangle k */
int _MMG2_split1_sim(MMG5_pMesh mesh, MMG5_pSol sol, int k, int vx[3]) {
  MMG5_pTria         pt,pt0;
  double             cal;
  unsigned char      tau[3];

  pt = &mesh->tria[k];
  pt0 = &mesh->tria[0];
  memcpy(pt0,pt,sizeof(MMG5_Tria));

  /* Set permutation from the reference configuration (case 1: edge 0 is splitted) to the actual one */
  tau[0] = 0; tau[1] = 1; tau[2] = 2;

  switch ( pt->flag ) {
  case 2:
    tau[0] = 1; tau[1] = 2; tau[2] = 0;
    break;

  case 4:
    tau[0] = 2; tau[1] = 0; tau[2] = 1;
    break;
  }

  pt0->v[tau[2]] = vx[tau[0]];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  pt0->v[tau[2]] = pt->v[tau[2]];
  pt0->v[tau[1]] = vx[tau[0]];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  return(1);
}

/* Split 1 edge of triangle k */
int _MMG2_split1(MMG5_pMesh mesh, MMG5_pSol sol, int k, int vx[3]) {
  MMG5_pTria       pt,pt1;
  MMG5_pPoint      p0;
  int              iel;
  unsigned char    tau[3];

  pt = &mesh->tria[k];

  /* Set permutation from the reference configuration (case 1: edge 0 is splitted) to the actual one */
  tau[0] = 0; tau[1] = 1; tau[2] = 2;

  switch ( pt->flag ) {
  case 2:
    tau[0] = 1; tau[1] = 2; tau[2] = 0;
    break;

  case 4:
    tau[0] = 2; tau[1] = 0; tau[2] = 1;
    break;
  }

  pt->flag = 0;

  /* Update of point references */
  p0 = &mesh->point[vx[tau[0]]];

  if ( pt->edg[tau[0]] > 0 )
    p0->ref = pt->edg[tau[0]];

  iel = _MMG2D_newElt(mesh);
  if ( !iel ) {
    _MMG2D_TRIA_REALLOC(mesh,iel,mesh->gap,
                        printf("  ## Error: unable to allocate a new element.\n");
                        _MMG5_INCREASE_MEM_MESSAGE();
                        printf("  Exit program.\n");
                        exit(EXIT_FAILURE)
      );
    pt = &mesh->tria[k];
  }
  pt1 = &mesh->tria[iel];
  memcpy(pt1,pt,sizeof(MMG5_Tria));

  /* Generic formulation for the split of one edge */
  /* Update of vertices */
  pt->v[tau[2]] = vx[tau[0]];
  pt1->v[tau[1]] = vx[tau[0]];

  /* Update of edge references and tags*/
  pt->tag[tau[1]] = MG_NOTAG;
  pt->edg[tau[1]] = 0;

  pt1->tag[tau[2]] = MG_NOTAG;
  pt1->edg[tau[2]] = 0;

  return(1);
}

/* Simulate the split of two edges in triangle k */
int _MMG2_split2_sim(MMG5_pMesh mesh, MMG5_pSol sol, int k, int vx[3]) {
  MMG5_pTria        pt,pt0;
  double            cal;
  unsigned char     tau[3];

  pt = &mesh->tria[k];
  pt0 = &mesh->tria[0];
  memcpy(pt0,pt,sizeof(MMG5_Tria));

  /* Set permutation from the reference configuration (case 6: edges 1,2 are splitted) to the actual one */
  tau[0] = 0; tau[1] = 1; tau[2] = 2;

  switch ( pt->flag ) {
  case 5:
    tau[0] = 1; tau[1] = 2; tau[2] = 0;
    break;

  case 3:
    tau[0] = 2; tau[1] = 0; tau[2] = 1;
    break;
  }

  pt0->v[tau[1]] = vx[tau[2]] ; pt0->v[tau[2]] = vx[tau[1]];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  pt0->v[tau[1]] = pt->v[tau[1]] ; pt0->v[tau[2]] = pt->v[tau[2]];
  pt0->v[tau[0]] = vx[tau[2]];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  pt0->v[tau[0]] = vx[tau[1]] ; pt0->v[tau[1]] = vx[tau[2]];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  return(1);
}

/* Split 2 edges of triangle k */
int _MMG2_split2(MMG5_pMesh mesh, MMG5_pSol sol, int k, int vx[3]) {
  MMG5_pTria       pt,pt1,pt2;
  MMG5_pPoint      p1,p2;
  int              iel,jel;
  unsigned char    tau[3];

  pt = &mesh->tria[k];

  /* Set permutation from the reference configuration (case 6: edges 1,2 are splitted) to the actual one */
  tau[0] = 0; tau[1] = 1; tau[2] = 2;

  switch ( pt->flag ) {
  case 5:
    tau[0] = 1; tau[1] = 2; tau[2] = 0;
    break;

  case 3:
    tau[0] = 2; tau[1] = 0; tau[2] = 1;
    break;
  }

  pt->flag = 0;

  /* Update of point references */
  p1 = &mesh->point[vx[tau[1]]];
  p2 = &mesh->point[vx[tau[2]]];

  if ( pt->edg[tau[1]] > 0 )
    p1->ref = pt->edg[tau[1]];

  if ( pt->edg[tau[2]] > 0 )
    p2->ref = pt->edg[tau[2]];

  iel = _MMG2D_newElt(mesh);
  if ( !iel ) {
    _MMG2D_TRIA_REALLOC(mesh,iel,mesh->gap,
                        printf("  ## Error: unable to allocate a new element.\n");
                        _MMG5_INCREASE_MEM_MESSAGE();
                        printf("  Exit program.\n");
                        exit(EXIT_FAILURE)
      );
    pt = &mesh->tria[k];
  }

  jel = _MMG2D_newElt(mesh);
  if ( !jel ) {
    _MMG2D_TRIA_REALLOC(mesh,jel,mesh->gap,
                        printf("  ## Error: unable to allocate a new element.\n");
                        _MMG5_INCREASE_MEM_MESSAGE();
                        printf("  Exit program.\n");
                        exit(EXIT_FAILURE)
      );
    pt = &mesh->tria[k];
  }

  pt1 = &mesh->tria[iel];
  pt2 = &mesh->tria[jel];
  memcpy(pt1,pt,sizeof(MMG5_Tria));
  memcpy(pt2,pt,sizeof(MMG5_Tria));


  /* Generic formulation for the split of two edges */
  /* Update of vertices */
  pt->v[tau[1]] = vx[tau[2]] ; pt->v[tau[2]] = vx[tau[1]];
  pt1->v[tau[0]] = vx[tau[2]];
  pt2->v[tau[0]] = vx[tau[1]]; pt2->v[tau[1]] = vx[tau[2]];

  /* Update of edge references and tags*/
  pt->tag[tau[0]] = MG_NOTAG;
  pt->edg[tau[0]] = 0;

  pt1->tag[tau[1]] = MG_NOTAG;
  pt1->edg[tau[1]] = 0;

  pt2->tag[tau[0]] = MG_NOTAG;   pt2->tag[tau[2]] = MG_NOTAG;
  pt2->edg[tau[0]] = MG_NOTAG;   pt2->edg[tau[2]] = MG_NOTAG;

  return(1);
}

/* Simulate the split of three edges in triangle k */
int _MMG2_split3_sim(MMG5_pMesh mesh, MMG5_pSol sol, int k, int vx[3]) {
  MMG5_pTria         pt,pt0;
  double             cal;

  pt = &mesh->tria[k];
  pt0 = &mesh->tria[0];
  memcpy(pt0,pt,sizeof(MMG5_Tria));

  pt0->v[1] = vx[2] ; pt0->v[2] = vx[1];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  pt0->v[0] = vx[2] ; pt0->v[1] = pt->v[1]; pt0->v[2] = vx[0];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  pt0->v[0] = vx[1] ; pt0->v[1] = vx[0]; pt0->v[2] = pt->v[2];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  pt0->v[1] = vx[2]; pt0->v[2] = vx[0];
  cal = _MMG2_quickcal(mesh,pt0);
  if ( cal < _MMG5_EPSD )  return(0);

  return(1);
}

/* Split the three edges of triangle k */
int _MMG2_split3(MMG5_pMesh mesh, MMG5_pSol sol, int k, int vx[3]) {
  MMG5_pTria          pt,pt1,pt2,pt3;
  MMG5_pPoint         p0,p1,p2;
  int                 iel,jel,kel;

  pt = &mesh->tria[k];
  pt->flag = 0;

  /* Update of point references */
  p0 = &mesh->point[vx[0]];
  p1 = &mesh->point[vx[1]];
  p2 = &mesh->point[vx[2]];

  if ( pt->edg[0] > 0 )
    p0->ref = pt->edg[0];

  if ( pt->edg[1] > 0 )
    p1->ref = pt->edg[1];

  if ( pt->edg[2] > 0 )
    p2->ref = pt->edg[2];

  iel = _MMG2D_newElt(mesh);
  if ( !iel ) {
    _MMG2D_TRIA_REALLOC(mesh,iel,mesh->gap,
                        printf("  ## Error: unable to allocate a new element.\n");
                        _MMG5_INCREASE_MEM_MESSAGE();
                        printf("  Exit program.\n");
                        exit(EXIT_FAILURE));

    pt = &mesh->tria[k];
  }

  jel = _MMG2D_newElt(mesh);

  if ( !jel ) {
    _MMG2D_TRIA_REALLOC(mesh,jel,mesh->gap,
                        printf("  ## Error: unable to allocate a new element.\n");
                        _MMG5_INCREASE_MEM_MESSAGE();
                        printf("  Exit program.\n");
                        exit(EXIT_FAILURE));
    pt = &mesh->tria[k];
  }

  kel = _MMG2D_newElt(mesh);

  if ( !kel ) {
    _MMG2D_TRIA_REALLOC(mesh,kel,mesh->gap,
                        printf("  ## Error: unable to allocate a new element.\n");
                        _MMG5_INCREASE_MEM_MESSAGE();
                        printf("  Exit program.\n");
                        exit(EXIT_FAILURE));
    pt = &mesh->tria[k];
  }

  pt1 = &mesh->tria[iel];
  pt2 = &mesh->tria[jel];
  pt3 = &mesh->tria[kel];
  memcpy(pt1,pt,sizeof(MMG5_Tria));
  memcpy(pt2,pt,sizeof(MMG5_Tria));
  memcpy(pt3,pt,sizeof(MMG5_Tria));

  /* Update of vertices */
  pt->v[1] = vx[2] ; pt->v[2] = vx[1];
  pt1->v[0] = vx[2] ; pt1->v[2] = vx[0];
  pt2->v[0] = vx[1]; pt2->v[1] = vx[0];
  pt3->v[0] = vx[1] ; pt3->v[1] = vx[2] ; pt3->v[2] = vx[0];

  /* Update of tags and references */
  pt->tag[0] = MG_NOTAG;
  pt->edg[0] = 0;

  pt1->tag[1] = MG_NOTAG;
  pt1->edg[1] = 0;

  pt2->tag[2] = MG_NOTAG;
  pt2->edg[2] = 0;

  pt3->tag[0] = pt3->tag[1] = pt3->tag[2] = MG_NOTAG;
  pt3->edg[0] = pt3->edg[1] = pt3->edg[2] = 0;

  return(1);
}