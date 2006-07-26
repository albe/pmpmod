/*
PMP Mod
Copyright (C) 2006 jonny

Homepage: http://jonny.leffe.dnsalias.com
E-mail:   jonny@leffe.dnsalias.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
color space conversion / tables generator
*/




/*
R = (Y - 16) * (255 / 219) + (V - 128) * (1.40200 * 255 / 224)
G = (Y - 16) * (255 / 219) - (U - 128) * (0.34414 * 255 / 224) - (V - 128) * (0.71414 * 255 / 224)
B = (Y - 16) * (255 / 219) + (U - 128) * (1.77200 * 255 / 224)


R = Y * (255 / 219) - 16 * (255 / 219) + V * (1.40200 * 255 / 224) - 128 * (1.40200 * 255 / 224)
G = Y * (255 / 219) - 16 * (255 / 219) - U * (0.34414 * 255 / 224) + 128 * (0.34414 * 255 / 224) - V * (0.71414 * 255 / 224) + 128 * (0.71414 * 255 / 224)
B = Y * (255 / 219) - 16 * (255 / 219) + U * (1.77200 * 255 / 224) - 128 * (1.77200 * 255 / 224)


R = Y * (255 / 219) + V * (1.40200 * 255 / 224) - 16 * (255 / 219) - 128 * (1.40200 * 255 / 224)
G = Y * (255 / 219) - U * (0.34414 * 255 / 224) - V * (0.71414 * 255 / 224) - 16 * (255 / 219) + 128 * (0.34414 * 255 / 224) + 128 * (0.71414 * 255 / 224)
B = Y * (255 / 219) + U * (1.77200 * 255 / 224) - 16 * (255 / 219) - 128 * (1.77200 * 255 / 224)


my   = (255.0 / 219.0)
mu1  = (1.77200 * 255.0 / 224.0)
mv1  = (1.40200 * 255.0 / 224.0)
mu2  = - (0.34414 * 255.0 / 224.0)
mv2  = - (0.71414 * 255.0 / 224.0)
addb = - 16.0 * (255.0 / 219.0) - 128.0 * (1.77200 * 255.0 / 224.0)
addr = - 16.0 * (255.0 / 219.0) - 128.0 * (1.40200 * 255.0 / 224.0)
addg = - 16.0 * (255.0 / 219.0) + 128.0 * (0.34414 * 255.0 / 224.0) + 128.0 * (0.71414 * 255.0 / 224.0)


B = Y * my + U * mu1 + addb
R = Y * my + V * mv1 + addr
G = Y * my + U * mu2 + V * mv2 + addg
*/


#include <stdio.h>


#define BIT 13


struct csc_struct
	{
	int R;
	int G;
	int B;
	};


struct minimum_maximum_struct
	{
	int first;
	int minimum;
	int maximum;
	};


int color_space_conversion_my [256];
int color_space_conversion_mu1[256];
int color_space_conversion_mv1[256];
int color_space_conversion_mu2[256];
int color_space_conversion_mv2[256];


int round_constant(double constant)
	{
	double multiplicator = (1 << BIT);

	constant *= multiplicator;

	if (constant >= 0)
		{
		return(constant + 0.5);
		}
	else
		{
		return(-(-constant + 0.5));
		}
	}


void make_csc(int Y, int U, int V, struct csc_struct *output)
	{
	int MU1  = color_space_conversion_mu1[U];
	int MU2  = color_space_conversion_mu2[U];
	int MV1  = color_space_conversion_mv1[V];
	int MV2  = color_space_conversion_mv2[V];
	int ADDG = MU2 + MV2;


	Y = color_space_conversion_my[Y];


	output->B = Y + MU1;
	output->R = Y + MV1;
	output->G = Y + ADDG;


	output->R >>= BIT;
	output->G >>= BIT;
	output->B >>= BIT;
	}


void find_minimum_maximum(int value, struct minimum_maximum_struct *minimum_maximum)
	{
	if (minimum_maximum->first)
		{
		minimum_maximum->first = 0;

		minimum_maximum->minimum = value;
		minimum_maximum->maximum = value;
		}
	else
		{
		if (value < minimum_maximum->minimum) minimum_maximum->minimum = value;
		if (value > minimum_maximum->maximum) minimum_maximum->maximum = value;
		}
	}


struct minimum_maximum_struct init_tables(int global_adder)
	{
	int my   = round_constant((255.0 / 219.0));
	int mu1  = round_constant((1.77200 * 255.0 / 224.0));
	int mv1  = round_constant((1.40200 * 255.0 / 224.0));
	int mu2  = round_constant(- (0.34414 * 255.0 / 224.0));
	int mv2  = round_constant(- (0.71414 * 255.0 / 224.0));
	int addb = round_constant(- 16.0 * (255.0 / 219.0) - 128.0 * (1.77200 * 255.0 / 224.0));
	int addr = round_constant(- 16.0 * (255.0 / 219.0) - 128.0 * (1.40200 * 255.0 / 224.0));
	int addg = round_constant(- 16.0 * (255.0 / 219.0) + 128.0 * (0.34414 * 255.0 / 224.0) + 128.0 * (0.71414 * 255.0 / 224.0));


	int i = 0;
	for (; i < 256; i++)
		{
		color_space_conversion_my[i] = i * my;
		}


	i = 0;
	for (; i < 256; i++)
		{
		color_space_conversion_mu1[i] = i * mu1 + addb + global_adder;
		}


	i = 0;
	for (; i < 256; i++)
		{
		color_space_conversion_mv1[i] = i * mv1 + addr + global_adder;
		}


	i = 0;
	for (; i < 256; i++)
		{
		color_space_conversion_mu2[i] = i * mu2;
		}


	i = 0;
	for (; i < 256; i++)
		{
		color_space_conversion_mv2[i] = i * mv2 + addg + global_adder;
		}




	struct minimum_maximum_struct minimum_maximum;
	minimum_maximum.first = 1;


	int Y = 0;
	for (; Y < 256; Y++)
		{


		int U = 0;
		for (; U < 256; U++)
			{


			int V = 0;
			for (; V < 256; V++)
				{


				struct csc_struct csc;
				make_csc(Y, U, V, &csc);


				find_minimum_maximum(csc.R, &minimum_maximum);
				find_minimum_maximum(csc.G, &minimum_maximum);
				find_minimum_maximum(csc.B, &minimum_maximum);
				}
			}
		}


	return(minimum_maximum);
	}


void show(char *txt, int *array)
	{
	printf("%s[256] = {", txt);


	int i = 0;
	for (; i < 256; i++)
		{
		if (i) printf(", ");
		printf("%i", array[i]);
		}


	printf("};\n");
	}


void show_fix(int length, int minimum)
	{
	printf("unsigned char color_space_conversion_fix[%i] = {", length);


	int i = 0;
	for (; i < length; i++)
		{
		int value = i + minimum;


		value = value <   0 ?   0 : value;
		value = value > 255 ? 255 : value;


		if (i) printf(", ");
		printf("%i", value);
		}


	printf("};\n");
	}


int main()
	{
	struct minimum_maximum_struct minimum_maximum = init_tables(0);
	printf("minimum: %i\n", minimum_maximum.minimum);
	printf("maximum: %i\n", minimum_maximum.maximum);
	printf("\n");


	struct minimum_maximum_struct new_minimum_maximum = init_tables((((-minimum_maximum.minimum) << 1) + 1) << (BIT - 1));
	printf("new_minimum: %i\n", new_minimum_maximum.minimum);
	printf("new_maximum: %i\n", new_minimum_maximum.maximum);
	printf("\n");


	show_fix(new_minimum_maximum.maximum + 1, minimum_maximum.minimum);


	show("int           color_space_conversion_my ", color_space_conversion_my);
	show("int           color_space_conversion_mu1", color_space_conversion_mu1);
	show("int           color_space_conversion_mv1", color_space_conversion_mv1);
	show("int           color_space_conversion_mu2", color_space_conversion_mu2);
	show("int           color_space_conversion_mv2", color_space_conversion_mv2);


	return(0);
	}
