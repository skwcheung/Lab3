/**
 * @file:   point.h
 * @brief:  header file for modeling a point in a 2D space
 * @date:   2015/09/13
 */

#ifndef POINT_H_
#define POINT_H_

struct point {
	int x;	/* x coordinate */
	int y;	/* y coordinate */
};

extern void set_position(int x, int y, struct point *buf);
extern int get_x_coord (struct point p);
extern int get_y_coord (struct point p);

#endif /* POINT_H_ */
