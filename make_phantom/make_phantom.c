/* ----------------------------- MNI Header -----------------------------------
   @NAME       : make_phantom.c
   @INPUT      : data used to define an object in a minc volume
                 
       usage:   make_phantom [options] outputfile.mnc
   
   @OUTPUT     : volume data containing either a voxelated ellipse or rectangle.

   @RETURNS    : TRUE if ok, ERROR if error.

   @COPYRIGHT  :
              Copyright 1993 Louis Collins, McConnell Brain Imaging Centre, 
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
   @CREATED    : Wed Mar 16 20:20:50 EST 1994  Louis Collins
   @MODIFIED   : $Log: make_phantom.c,v $
   @MODIFIED   : Revision 1.5  2004-02-12 05:53:40  rotor
   @MODIFIED   :  * removed public/private defs
   @MODIFIED   :
   @MODIFIED   : Revision 1.4  2000/02/20 04:00:58  stever
   @MODIFIED   : * use new history_string() function to generate history strings
   @MODIFIED   :   when outputting MNI files (.mnc, .xfm)
   @MODIFIED   : * removed unused vax routines from Proglib
   @MODIFIED   : * tuned configure script; CPPFLAGS and LDFLAGS are now left alone,
   @MODIFIED   :   for the installer to use
   @MODIFIED   :
   @MODIFIED   : Revision 1.3  2000/02/15 19:01:59  stever
   @MODIFIED   : Add tests for param2xfm, minctracc -linear.
   @MODIFIED   :
   @MODIFIED   : Revision 1.2  2000/02/02 20:10:13  stever
   @MODIFIED   : * minctracc/Testing was a copy of Testing with one extra test only;
   @MODIFIED   :   folded the extra test into Testing, and removed minctracc/Testing
   @MODIFIED   : * minor source changes to placate GCC's -Wall option
   @MODIFIED   :
   @MODIFIED   : Revision 1.1  1996/08/21 15:25:05  louis
   @MODIFIED   : Initial revision
   @MODIFIED   :
 * Revision 1.1  94/03/16  22:58:40  louis
 * Initial revision
 * 
---------------------------------------------------------------------------- */

#include <config.h>
#include <volume_io.h>
#include <Proglib.h>
#include "make_phantom.h"

static char *default_dim_names[N_DIMENSIONS] = { MIxspace, MIyspace, MIzspace };

#define SUBSTEPS  9

void  set_min_max(Real *voxel, 
			  int *minx, int *maxx, 
			  int *miny, int *maxy, 
			  int *minz, int *maxz)
{
  int x,y,z;

  /* If voxel[X] = 0.6, then *minx will be set to 1.
     Is that correct, or do we want *minx = 0 ? */
  x = ROUND(voxel[X]);
  y = ROUND(voxel[Y]);
  z = ROUND(voxel[Z]);
  
  if (x > *maxx) *maxx = x;
  if (x < *minx) *minx = x;

  if (y > *maxy) *maxy = y;
  if (y < *miny) *miny = y;

  if (z > *maxz) *maxz = z;
  if (z < *minz) *minz = z;
}

void  set_min_max_r(Real *voxel, 
			    Real *minx, Real *maxx, 
			    Real *miny, Real *maxy, 
			    Real *minz, Real *maxz)
{
  if (voxel[X] > *maxx) *maxx = voxel[X];
  if (voxel[X] < *minx) *minx = voxel[X];

  if (voxel[Y] > *maxy) *maxy = voxel[Y];
  if (voxel[Y] < *miny) *miny = voxel[Y];

  if (voxel[Z] > *maxz) *maxz = voxel[Z];
  if (voxel[Z] < *minz) *minz = voxel[Z];
}


Real partial_elliptical(Volume data, Real center[], Real r2[], Real step[], 
			       int vx, int vy, int vz)
{
  Real istep,jstep,kstep,coord[3];
  int total,total_in,i,j,k,m;
  Real frac;

  total = total_in = 0;

  for_inclusive(i,0,SUBSTEPS) {

    istep = -0.5 + ((Real)i/(Real)SUBSTEPS);

    for_inclusive(j,0,SUBSTEPS) {

      jstep = -0.5 + ((Real)j/(Real)SUBSTEPS);


      for_inclusive(k,0,SUBSTEPS){
  
	kstep = -0.5 + ((Real)k/(Real)SUBSTEPS);

	convert_3D_voxel_to_world(data, 
				  ((Real)vx+istep), ((Real)vy+jstep), ((Real)vz+kstep), 
				  &coord[X], &coord[Y], &coord[Z]);
	
	for_less(m,0,3) coord[m] = center[m] - coord[m];
	for_less(m,0,3) coord[m] = coord[m]*coord[m] / r2[m];
	
	if ( (coord[X] + coord[Y] + coord[Z]) <= 1.0000001 ) 
	  total_in++;

	total++;
      }
    }
  }

  if (total > 0)
    frac = (Real)total_in/(Real)total;
  else
    frac = 0.0;


/*
if (frac>0.0);
  print("tot_in = %3d, tot = %3d, frac = %f\n",total_in, total, frac);
*/


  return(frac);
}


BOOLEAN is_inside_ellipse(Volume data, Real center[], Real r2[], int i, int j, int k)
{
  Real coord[3];
  int m;

  convert_3D_voxel_to_world(data, 
			    (Real)i, (Real)j, (Real)k, 
			    &coord[X], &coord[Y], &coord[Z]);

  for_less(m,0,3) coord[m] = center[m] - coord[m];
  for_less(m,0,3) coord[m] = coord[m]*coord[m] / r2[m];
  
  return (coord[X] + coord[Y] + coord[Z]) <= 1.0000001;
}


int main (int argc, char *argv[] )
{   
  FILE 
    *ofd;
  char 
    *outfilename;  
  Status 
    status;
  
  Volume
    data;
  Real
    fraction,zero, one,
    r2[3],
    coord[3],
    voxel[3];
  char *history;
  Real
    w_minz,w_maxz,
    w_miny,w_maxy,
    w_minx,w_maxx;
  int 
    in1,in2,in3,in4,in5,in6,in7,
    minz,maxz,
    miny,maxy,
    minx,maxx,
    i,j,k,m;

  /* set default values */

  prog_name = argv[0];
  outfilename = NULL;

  history = history_string(argc, argv);

  /* Call ParseArgv to interpret all command line args */

  if (ParseArgv(&argc, argv, argTable, 0) || (argc!=2)) {
    (void) fprintf(stderr, 
		   "\nUsage: %s [<options>] output.mnc\n", 
		   prog_name);
    (void) fprintf(stderr,"       %s [-help]\n\n", prog_name);
    exit(EXIT_FAILURE);
  }

  outfilename  = argv[1];	/* set up necessary file names */

  if (!clobber_flag && file_exists(outfilename)) {
    print ("File %s exists.\n",outfilename);
    print ("Use -clobber to overwrite.\n");
    return ERROR;
  }

				/* check to see if the output file can be written */
  status = open_file( outfilename , WRITE_FILE, BINARY_FORMAT,  &ofd );
  if ( status != OK ) {
    print ("filename `%s' cannot be opened.", outfilename);
    return ERROR;
  }
  status = close_file(ofd);
  (void)remove(outfilename);   

  /******************************************************************************/
  /*             create volume data                                             */
  /******************************************************************************/
  
  data = create_volume(3, default_dim_names, datatype, is_signed, 0.0, 0.0);
  set_volume_voxel_range(data, voxel_range[0], voxel_range[1]);
  set_volume_real_range( data, real_range[0],  real_range[1]);
  set_volume_sizes(data, count);
  set_volume_separations(data, step);

  voxel[0] = 0.0;
  voxel[1] = 0.0;
  voxel[2] = 0.0;

  set_volume_translation(data, voxel, start);
  alloc_volume_data(data);

  zero = CONVERT_VALUE_TO_VOXEL(data, background);
  one = CONVERT_VALUE_TO_VOXEL(data, fill_value);

  if (debug) print ("zero: real = %f, voxel = %f\n", background, zero);
  if (debug) print ("one: real = %f, voxel = %f\n", fill_value, one);


  /******************************************************************************/
  /*             write out background value                                     */
  /******************************************************************************/

  for_less(i,0,count[X])
    for_less(j,0,count[Y])
      for_less(k,0,count[Z]){
	SET_VOXEL_3D(data, i,j,k, zero);
      }

  /******************************************************************************/
  /*             build object data                                              */
  /******************************************************************************/

				/* begin by finding bounding box for data */
  maxx = maxy = maxz = -INT_MAX;
  minx = miny = minz = INT_MAX;
  w_maxx = w_maxy = w_maxz = -DBL_MAX;
  w_minx = w_miny = w_minz = DBL_MAX;

  for_less(i,0,3) coord[i] = center[i];
  convert_3D_world_to_voxel(data, 
			    coord[X], coord[Y], coord[Z], 
			    &voxel[X], &voxel[Y], &voxel[Z]);  
  set_min_max(voxel, &minx, &maxx, &miny, &maxy, &minz, &maxz);
  set_min_max_r(coord, &w_minx, &w_maxx, &w_miny, &w_maxy, &w_minz, &w_maxz);
  if (debug) print ("%f %f %f\n",voxel[X], voxel[Y], voxel[Z]);

  for_less(i,0,3) coord[i] = center[i]; coord[Z] += width[Z]/2.0;
  convert_3D_world_to_voxel(data, 
			    coord[X], coord[Y], coord[Z], 
			    &voxel[X], &voxel[Y], &voxel[Z]);  
  if (debug) print ("%f %f %f\n",voxel[X], voxel[Y], voxel[Z]);
  set_min_max(voxel, &minx, &maxx, &miny, &maxy, &minz, &maxz);
  set_min_max_r(coord, &w_minx, &w_maxx, &w_miny, &w_maxy, &w_minz, &w_maxz);

  for_less(i,0,3) coord[i] = center[i]; coord[Z] -= width[Z]/2.0;
  convert_3D_world_to_voxel(data, 
			    coord[X], coord[Y], coord[Z], 
			    &voxel[X], &voxel[Y], &voxel[Z]);  
  if (debug) print ("%f %f %f\n",voxel[X], voxel[Y], voxel[Z]);
  set_min_max(voxel, &minx, &maxx, &miny, &maxy, &minz, &maxz);
  set_min_max_r(coord, &w_minx, &w_maxx, &w_miny, &w_maxy, &w_minz, &w_maxz);

  for_less(i,0,3) coord[i] = center[i]; coord[Y] += width[Y]/2.0;
  convert_3D_world_to_voxel(data, 
			    coord[X], coord[Y], coord[Z], 
			    &voxel[X], &voxel[Y], &voxel[Z]);  
  if (debug) print ("%f %f %f\n",voxel[X], voxel[Y], voxel[Z]);
  set_min_max(voxel, &minx, &maxx, &miny, &maxy, &minz, &maxz);
  set_min_max_r(coord, &w_minx, &w_maxx, &w_miny, &w_maxy, &w_minz, &w_maxz);

  for_less(i,0,3) coord[i] = center[i]; coord[Y] -= width[Y]/2.0;
  convert_3D_world_to_voxel(data, 
			    coord[X], coord[Y], coord[Z], 
			    &voxel[X], &voxel[Y], &voxel[Z]);  
  if (debug) print ("%f %f %f\n",voxel[X], voxel[Y], voxel[Z]);
  set_min_max(voxel, &minx, &maxx, &miny, &maxy, &minz, &maxz);
  set_min_max_r(coord, &w_minx, &w_maxx, &w_miny, &w_maxy, &w_minz, &w_maxz);

  for_less(i,0,3) coord[i] = center[i]; coord[X] += width[X]/2.0;
  convert_3D_world_to_voxel(data, 
			    coord[X], coord[Y], coord[Z], 
			    &voxel[X], &voxel[Y], &voxel[Z]);  
  if (debug) print ("%f %f %f\n",voxel[X], voxel[Y], voxel[Z]);
  set_min_max(voxel, &minx, &maxx, &miny, &maxy, &minz, &maxz);
  set_min_max_r(coord, &w_minx, &w_maxx, &w_miny, &w_maxy, &w_minz, &w_maxz);

  for_less(i,0,3) coord[i] = center[i]; coord[X] -= width[X]/2.0;
  convert_3D_world_to_voxel(data, 
			    coord[X], coord[Y], coord[Z], 
			    &voxel[X], &voxel[Y], &voxel[Z]);  
  if (debug) print ("%f %f %f\n",voxel[X], voxel[Y], voxel[Z]);
  set_min_max(voxel, &minx, &maxx, &miny, &maxy, &minz, &maxz);
  set_min_max_r(coord, &w_minx, &w_maxx, &w_miny, &w_maxy, &w_minz, &w_maxz);

  if (debug) print ("mins, maxs: %3d %3d %3d %3d %3d %3d\n", minx, maxx, miny, maxy, minz, maxz);


				/* check to see if limits of object fit
				   in data volume */

  if ( maxx < 0  || maxy < 0 || maxz < 0 ||
       minx > count[X] || miny > count[Y] || minz > count[Z]) {
    print ("There is no voxel overlap between object and volume as defined.\n");
    return ERROR;
  }

				/* add one voxel all around, for partial volume 
				   calculations. */
  if (object==ELLIPSE) {
    --minx;  --miny;  --minz;
    ++maxx;  ++maxy;  ++maxz;
  }
				/* readjust for edge of volume */
  if (minx < 0) minx = 0;
  if (maxx > count[X]-1) maxx = count[X]-1;
  if (miny < 0) miny = 0;
  if (maxy > count[Y]-1) maxy = count[Y]-1;
  if (minz < 0) minz = 0;
  if (maxz > count[Z]-1) maxz = count[Z]-1;


  if (debug) print ("mins, maxs: %3d %3d %3d %3d %3d %3d\n", minx, maxx, miny, maxy, minz, maxz);

  if (object==RECTANGLE) {
    for_inclusive(i, minx, maxx)
      for_inclusive(j, miny, maxy)
	for_inclusive(k, minz, maxz) {
	  SET_VOXEL_3D(data, i,j,k, one);
	}
  } else {			/* ellipsoid */

    for_less(m,0,3) 
      r2[m] = width[m]*width[m]/4.0;

    for_inclusive(i, minx, maxx)
      for_inclusive(j, miny, maxy)
	for_inclusive(k, minz, maxz) {

	  in1 = is_inside_ellipse(data, center, r2, i+1,j,  k );
	  in2 = is_inside_ellipse(data, center, r2, i-1,j,  k);
	  in3 = is_inside_ellipse(data, center, r2, i,  j+1,k);
	  in4 = is_inside_ellipse(data, center, r2, i,  j-1,k);
	  in5 = is_inside_ellipse(data, center, r2, i,  j,  k+1);
	  in6 = is_inside_ellipse(data, center, r2, i,  j,  k-1);
	  in7 = is_inside_ellipse(data, center, r2, i,  j,  k);
	  
	  if (in1 && in2 && in3 && in4 && in5 && in6 && in7) {
	    SET_VOXEL_3D(data, i,j,k, one);
	  }
	  else {
	    if (in1 || in2 || in3 || in4 || in5 || in6 || in7) {

	      if (partial_flag) {
		Real value;
		fraction = partial_elliptical(data, center, r2, step, i,  j,  k);
		fraction = fraction*edge_value;
		value = CONVERT_VALUE_TO_VOXEL(data, fraction);
		SET_VOXEL_3D(data, i,j,k, value);
	      }
	      else {
		  if (in7) {
		      SET_VOXEL_3D(data, i,j,k, one);
		  }
	      }
	      
	    }
	  }	  
    }
    
  }
  
  
  status = output_volume(outfilename, NC_UNSPECIFIED, FALSE, 0.0, 0.0, data, 
			 history, (minc_output_options *)NULL);  
  if (status != OK)
    print("problems writing volume data for %s.", outfilename);
  
  
  return(status);
   
}

