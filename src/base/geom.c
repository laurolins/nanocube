//----------------------------------------------------------------------------------
//
// math stuff
//
//----------------------------------------------------------------------------------

// @todo f32 vs f64?
//3.14159265358979323846f

typedef union {
	f32 e[2];
	struct { f32 x, y; };
	struct { f32 u, v; };
	struct { f32 lon, lat; }; // name for degrees
	struct { f32 mx, my; }; // name for mercator
} v2;

internal v2 v2_add(v2 a, v2 b) { return (v2) { .x = a.x + b.x, .y = a.y + b.y }; }
internal v2 v2_x() { return (v2) { .x = 1.0f, .y = 0.0f }; }
internal v2 v2_y() { return (v2) { .x = 0.0f, .y = 1.0f }; }
internal v2 v2_sub(v2 a, v2 b) { return (v2) { .x = a.x - b.x, .y = a.y - b.y }; }
internal v2 v2_scale(v2 a, f32 s) { return (v2) { .x = s * a.x, .y = s * a.y }; }
internal v2 v2_hadamard(v2 a, v2 b) { return (v2) { .x = a.x*b.x, .y = a.y*b.y }; }
internal f32 v2_dot(v2 a, v2 b) { return a.x * b.x + a.y * b.y; }
internal f32 v2_length(v2 a) { return pt_sqrt_f32(a.x * a.x + a.y * a.y); }
internal v2 v2_normalized(v2 a) { return v2_scale(a, 1.0f/v2_length(a)); }

// @note that two normalizations occur here (maybe we don't need this)
internal f32 v2_angle_slow(v2 a, v2 b) { return pt_acos_f32(v2_dot(v2_normalized(a),v2_normalized(b))); }

typedef union {
	s32 e[2];
	struct { s32 x, y; };
} v2i;

typedef union {
	s32 e[4];
	struct {
		s32 x, y;
		union {
			struct { s32 w, z; };
			struct { s32 width, height; };
		};
	};
} v4i;


// a bbox with 0 width is considered empty
internal v4i
v4i_bbox(v4i a, v4i b)
{
	if (a.width == 0 && b.width == 0) {
		return (v4i) { 0 };
	} else if (a.width == 0) {
		return b;
	} else if (b.width == 0) {
		return a;
	} else {
		s32 x0 = MIN(a.x,b.x);
		s32 y0 = MIN(a.y,b.y);
		s32 x1 = MAX(a.x+a.width,b.x+b.width);
		s32 y1 = MAX(a.y+a.height,b.y+b.height);
		return (v4i) { .x = x0, .y = y0, .width=x1-x0, .height=y1-y0 };
	}
}

internal v4i
v4i_intersection(v4i a, v4i b)
{
	s32 x0 = MAX(a.x,b.x);
	s32 x1 = MIN(a.x+a.width,b.x+b.width);
	s32 y0 = MAX(a.y,b.y);
	s32 y1 = MIN(a.y+a.height,b.y+b.height);
	if (x0 < x1 && y0 < y1) {
		return (v4i) { .x=x0,.y=y0,.width=x1-x0, .height=y1-y0 };
	} else {
		return (v4i) { 0 };
	}
}

internal v4i
v4i_bbox_translate(v4i a, v2i p)
{
	return (v4i) { .x = a.x + p.x, .y = a.y + p.y, .width = a.width, .height = a.height };
}

internal v4i
v4i_bbox_add_point(v4i a, v2i p)
{
	if (a.width == 0) {
		Assert(a.height == 0);
		// if it is empty one pixel bbox
		return (v4i) { .x=p.x, .y=p.y, .width=1, .height=1 };
	} else {
		s32 min_x = MIN(a.x,p.x);
		s32 min_y = MIN(a.y,p.y);
		s32 max_x = MAX(a.x + a.width,  p.x);
		s32 max_y = MAX(a.y + a.height, p.y);
		return (v4i) { .x = min_x, .y = min_y, .width=max_x-min_x+1, .height=max_y-min_y+1 };
	}
}



internal s32 geom_hit_rect(v4i rect, v2i p) { return p.x >= rect.x && p.y >= rect.y && p.x < rect.x + rect.width && p.y < rect.y + rect.height; }

internal v2i v2i_min(v2i a, v2i b) { return (v2i) { .x = MIN(a.x, b.x), .y = MIN(a.y,b.y) }; }
internal v2i v2i_max(v2i a, v2i b) { return (v2i) { .x = MAX(a.x, b.x), .y = MAX(a.y,b.y) }; }
internal s32 v2i_orient(v2i a, v2i b, v2i c) { return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x); }
internal b8 v2i_is_top_left(v2i a, v2i b) {
	// in a counter clock-wise triangle
	// 1. left edge is one that goes down (a.x > b.x)
	// 2. top edge is one that is horizontal and goes to the left (a.x > b.x)
	return (a.y > b.y || (a.y == b.y && a.x > b.x));
}
internal s32 v2i_equal(v2i a, v2i b) { return a.x == b.x && a.y == b.y; }


typedef union {
	s32 e[3];
	struct { s32 x, y, z; };
	struct { s32 w0, w1, w2; };
} v3i;
internal v3i v3i_add(v3i a, v3i b) { return (v3i) { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z }; }

typedef union {
	f32 e[4];
	struct { f32 x, y, z;    };
	struct { f32 u, v, i0_;  };
	struct { f32 r, g, b;    };
	struct { v2 xy; f32 i1_; };
	struct { f32 i2_; v2 yz; };
	struct { v2 uv; f32 i3_; };
	struct { f32 i4_; v2 v_; };
	struct { f32 radius, polar_angle, azimuth_angle; }; // spherical coordinates
} v3;

internal v3 v3_zero() { return (v3) { .x = 0.0f, .y = 0.0f, .z = 0.0f }; }
internal v3 v3_x() { return (v3) { .x = 1.0f, .y = 0.0f, .z = 0.0f }; }
internal v3 v3_y() { return (v3) { .x = 0.0f, .y = 1.0f, .z = 0.0f }; }
internal v3 v3_z() { return (v3) { .x = 0.0f, .y = 0.0f, .z = 1.0f }; }
internal v3 v3_x00(f32 x) { return (v3) { .x = x, .y = 0, .z = 0 }; }
internal v3 v3_0y0(f32 y) { return (v3) { .x = 0, .y = y, .z = 0 }; }
internal v3 v3_00z(f32 z) { return (v3) { .x = 0, .y = 0, .z = z }; }
internal v3 v3_xy0(f32 x, f32 y) { return (v3) { .x = x, .y = y, .z = 0 }; }
internal v3 v3_x0z(f32 x, f32 z) { return (v3) { .x = x, .y = 0, .z = z }; }
internal v3 v3_0yz(f32 y, f32 z) { return (v3) { .x = 0, .y = y, .z = z }; }
internal v3 v3_rep(f32 v) { return (v3) { .x = v, .y = v, .z = v }; }
internal v3 v3_neg(v3 a) { return (v3) { .x = -a.x, .y = -a.y, .z = -a.z }; }
internal v3 v3_add(v3 a, v3 b) { return (v3) { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z }; }
internal v3 v3_sub(v3 a, v3 b) { return (v3) { .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z }; }
internal v3 v3_scale(v3 a, f32 s) { return (v3) { .x = s * a.x, .y = s * a.y, .z = s * a.z }; }
internal v3 v3_inv_scale(v3 a, f32 s) { return (v3) { .x = a.x/s, .y = a.y/s, .z = a.z/s }; }
internal v3 v3_hadamard(v3 a, v3 b) { return (v3) { .x = a.x*b.x, .y = a.y*b.y, .z = a.z*b.z }; }
internal f32 v3_sum_entries(v3 a) { return a.x + a.y + a.z; }
internal f32 v3_inner(v3 a, v3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
internal f32 v3_length_sq(v3 a) { return v3_inner(a,a); }
internal f32 v3_length(v3 a) { return pt_sqrt_f32(v3_inner(a,a)); }
internal v3 v3_normalize(v3 a) { return v3_scale(a, 1.0f/v3_length(a)); }
internal v3 v3_normalize_or_zero(v3 a)
{
	f32 len_sq = v3_length_sq(a);
	if (len_sq > 0.0) {
		return v3_scale(a, 1.0f/pt_sqrt_f32(len_sq));
	} else {
		return (v3) { .x = 0.0f, .y = 0.0f };
	}
}
internal s32 v3_is_inside_normalized_cube(v3 a) { return a.x >= -1.0f && a.x <= 1.0f && a.y >= -1.0f && a.y <= 1.0f && a.z >= -1.0f && a.z <= 1.0f; }
internal f32 v3_angle(v3 a, v3 b, v3 c) { return pt_acos_f32(v3_inner(v3_normalize(v3_sub(b,a)),v3_normalize(v3_sub(c,a)))); }

typedef union {
	f32     e[4];
	struct {
		union {
			v3 xyz;
			v3 rgb;
			struct { f32 r, g, b; };
			struct {
				f32 x, y;
				union {
					f32 z;
					f32 width;
				};
			};
		};
		union {
			f32 w;
			f32 a;
			f32 height;
		};
	};
	struct { v2  xy; f32 i0_; f32 i1_; };
	struct { f32 i2_; v2  yz; f32 i3_; };
	struct { f32 i4_; f32 i5_; v2 zw; };
	struct { f32 lon0, lat0, lon1, lat1; };
} v4;

// there is no notion of empty bbox using v4
static v4
v4_bbox_add_point(v4 bbox, v2 p)
{
	v2 max = { .x = bbox.x + bbox.width, .y = bbox.y + bbox.height };
	bbox.x = MIN(bbox.x, p.x);
	bbox.y = MIN(bbox.y, p.y);
	max.x = MAX(max.x, p.x);
	max.y = MAX(max.y, p.y);
	bbox.width = max.x - bbox.x;
	bbox.height= max.y - bbox.y;
	return bbox;
}

internal v4 v4_add(v4 a, v4 b) { return (v4) { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z, .w = a.w + b.w }; }
internal v4 v4_sub(v4 a, v4 b) { return (v4) { .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z, .w = a.w - b.w }; }
internal v4 v4_scale(v4 a, f32 s) { return (v4) { .x = s * a.x, .y = s * a.y, .z = s * a.z, .w = s * a.w }; }
internal v4 v4_lerp(v4 c1, v4 c2, f32 alpha) { return v4_add(v4_scale(c1,1.0f-alpha), v4_scale(c2,alpha)); }
internal v4 v4_divide_by_w(v4 a) { return (v4) { .x = a.x/a.w, .y=a.y/a.w, .z=a.z/a.w, .w=1.0f }; }
internal v4 v4_hadamard(v4 a, v4 b) { return (v4) { .x = a.x*b.x, .y = a.y*b.y, .z = a.z*b.z, .w = a.w*b.w }; }
internal v4 v4_clamp(v4 a, f32 min, f32 max) { return (v4) { .x = MIN(MAX(min,a.x),max), .y = MIN(MAX(min,a.y),max), .z = MIN(MAX(min,a.y),max), .w = MIN(MAX(min,a.w),max)}; }

// internal v3 v3_cross(v3 a, v3 b) {
// 	// f32x4 r = f32x4_cross_4shuffles(*((f32x4*)&a),*((f32x4*)&b));
// 	f32x4 r = f32x4_cross_3shuffles(*((f32x4*)&a),*((f32x4*)&b));
// 	return *((v3*) &r);
// }
internal v3 v3_cross(v3 a, v3 b) { return (v3) { .x = a.y*b.z - a.z*b.y, .y = a.z*b.x - a.x*b.z, .z = a.x*b.y - a.y*b.x }; }
internal v3  v3_plane_normal(v3 a, v3 b, v3 c) { return v3_normalize(v3_cross(v3_sub(b,a),v3_sub(c,a))); }

internal v4
geom_rect_intersection_v4_v4i(v4 a, v4i b)
{
	f32 x0 = MAX(a.x,(f32)b.x);
	f32 x1 = MIN(a.x+a.width,(f32) (b.x+b.width));
	f32 y0 = MAX(a.y,(f32)b.y);
	f32 y1 = MIN(a.y+a.height,(f32)(b.y+b.height));
	if (x0 < x1 && y0 < y1) {
		return (v4) { .x=x0, .y=y0, .width=x1-x0, .height=y1-y0 };
	} else {
		return (v4) { 0 };
	}
}

typedef union {
	// NOTE(casey): These are stored ROW MAJOR - E[ROW][COLUMN]!!!
	f32 e[4][4];
	f32 raw[16];
} m4;

internal m4
m4_identity()
{
	return (m4) { .e = {
		{  1, 0, 0, 0},
		{  0, 1, 0, 0},
		{  0, 0, 1, 0},
		{  0, 0, 0, 1}
	} };
}

internal m4
m4_mul(m4 a, m4 b)
{
	m4 result;
	pt_clear(result);
	for(s32 r = 0; r < 4; ++r) {
		for(s32 c = 0; c < 4; ++c) {
			for(s32 i = 0; i < 4; ++i) {
				result.e[r][c] += a.e[r][i]*b.e[i][c];
			}
		}
	}
	return result;
}

internal m4
m4_translate(m4 a, v3 t)
{
	m4 result = a;
	result.e[0][3] += t.x;
	result.e[1][3] += t.y;
	result.e[2][3] += t.z;
	return result;
}

internal m4
m4_translation(v3 t)
{
	return (m4) { .e = {
		{  1, 0, 0, t.x},
		{  0, 1, 0, t.y},
		{  0, 0, 1, t.z},
		{  0, 0, 0, 1}
	} };
}

internal m4
m4_scale(v3 s)
{
	return (m4) { .e = {
		{  s.x,   0,   0, 0},
		{  0,   s.y,   0, 0},
		{  0,     0, s.z, 0},
		{  0,     0,   0, 1}
	} };
}

internal m4
m4_rotation(s32 axis, f32 theta)
{
	// axis == 0 -> x
	// axis == 1 -> y
	// axis == 2 -> z
// 	static const f32 deg_to_rad = pt_PI / 180.0f;
// 	if (theta_in_degrees) {
// 		theta = theta * deg_to_rad;
// 	}
	f32 sin_theta = pt_sin_f32(theta);
	f32 cos_theta = pt_cos_f32(theta);

	if (axis == 0) {
		return (m4) { .e = {
			{  1,         0,          0, 0},
			{  0, cos_theta, -sin_theta, 0},
			{  0, sin_theta,  cos_theta, 0},
			{  0,         0,          0, 1}
		} };
	} else if (axis == 1) {
		return (m4) { .e = {
			{  cos_theta, 0, -sin_theta, 0},
			{          0, 1,          0, 0},
			{  sin_theta, 0,  cos_theta, 0},
			{          0, 0,          0, 1}
		} };
	} else if (axis == 2) {
		return (m4) { .e = {
			{  cos_theta, -sin_theta, 0, 0},
			{  sin_theta,  cos_theta, 0, 0},
			{          0,          0, 1, 0},
			{          0,          0, 0, 1}
		} };
	}
	InvalidCodePath;
	return m4_identity();
}

internal m4
m4_from_rows(v3 x, v3 y, v3 z)
{
	return (m4) { .e = {
		{x.x, x.y, x.z, 0},
		{y.x, y.y, y.z, 0},
		{z.x, z.y, z.z, 0},
		{  0,   0,   0, 1}
	} };
}

internal m4
m4_from_columns(v3 x, v3 y, v3 z)
{
	return (m4) { .e = {
		{x.x, y.x, z.x, 0},
		{x.y, y.y, z.y, 0},
		{x.z, y.z, z.z, 0},
		{  0,   0,   0, 1}
	} };
}


internal m4
m4_rotation_through_a_plane(v3 plane, f32 theta)
{
	// change of basis
	v3 new_basis_z = v3_normalize(plane);
	// v3 up = (v3) { .x = new_basis_z.x, .y = new_basis_z.y + 1, .z = new_basis_z.z };;
	v3 up = (v3) { .x = -new_basis_z.z, .y = +new_basis_z.x, .z = -new_basis_z.y };
	// y and z will be anything
	v3 new_basis_x = v3_normalize(v3_cross(up, new_basis_z));
	v3 new_basis_y = v3_cross(new_basis_z, new_basis_x);
	return m4_mul(m4_from_columns(new_basis_x, new_basis_y, new_basis_z),
		     m4_mul(m4_rotation(2,theta), m4_from_rows(new_basis_x, new_basis_y, new_basis_z)));
}

internal m4
m4_rotation_through_a_plane_with_offset(v3 plane, v3 offset, f32 theta)
{
	// change of basis
	v3 new_basis_z = v3_normalize(plane);
	// any different vector to be the up
	v3 up = (v3) { .x = -new_basis_z.z, .y = +new_basis_z.x, .z = -new_basis_z.y };
	// y and z will be anything
	v3 new_basis_x = v3_normalize(v3_cross(up, new_basis_z));
	v3 new_basis_y = v3_cross(new_basis_z, new_basis_x);
	return m4_mul(m4_translation(offset),
		      m4_mul(m4_from_columns(new_basis_x, new_basis_y, new_basis_z),
			     m4_mul(m4_rotation(2,theta),
				    m4_mul(m4_from_rows(new_basis_x, new_basis_y, new_basis_z),
					   m4_translation(v3_neg(offset))))));
}

internal v4
m4_transform_v4(m4 a, v4 p)
{
	return (v4) {
		.x = p.x*a.e[0][0] + p.y*a.e[0][1] + p.z*a.e[0][2] + p.w*a.e[0][3],
		.y = p.x*a.e[1][0] + p.y*a.e[1][1] + p.z*a.e[1][2] + p.w*a.e[1][3],
		.z = p.x*a.e[2][0] + p.y*a.e[2][1] + p.z*a.e[2][2] + p.w*a.e[2][3],
		.w = p.x*a.e[3][0] + p.y*a.e[3][1] + p.z*a.e[3][2] + p.w*a.e[3][3]
	};
}

internal v2i
v2i_add(v2i a, v2i b)
{
	return (v2i) { .x = a.x + b.x, .y = a.y + b.y };
}

internal v2i
v2i_sub(v2i a, v2i b)
{
	return (v2i) { .x = a.x - b.x, .y = a.y - b.y };
}

internal v4i
geom_rect_translate_v4i(v4i rect, v2i t)
{
	return (v4i) { .x = rect.x + t.x, .y = rect.y + t.y, .width = rect.width, .height = rect.height } ;
}

static v4i
geom_insets_rect_v4i(v4i rect, v4i insets)
{
	return (v4i) { .x = rect.x+insets.x, .y=rect.y+insets.y,
		.width=rect.width - insets.x - insets.z,
		.height = rect.height - insets.y - insets.w };
}

internal v4
m4_transform_and_divide_by_w(m4 a, v4 p)
{
	f32 one_over_w = 1.0f / (p.x*a.e[3][0] + p.y*a.e[3][1] + p.z*a.e[3][2] + p.w*a.e[3][3]);
	return (v4) {
		.x = (p.x*a.e[0][0] + p.y*a.e[0][1] + p.z*a.e[0][2] + p.w*a.e[0][3]) * one_over_w,
		.y = (p.x*a.e[1][0] + p.y*a.e[1][1] + p.z*a.e[1][2] + p.w*a.e[1][3]) * one_over_w,
		.z = (p.x*a.e[2][0] + p.y*a.e[2][1] + p.z*a.e[2][2] + p.w*a.e[2][3]) * one_over_w,
		.w = 1.0f
	};
}

static v4
geom_insets_rect_v4_val(v4 rect, f32 val)
{
	return (v4) { .x = rect.x+val, .y=rect.y+val, .width=rect.width - 2*val, .height= rect.height - 2*val };
}

// static v4
// geom_insets_rect_v4(v4 rect, v4 insets)
// {
// 	return (v4) { .x = rect.x + insets.x, .y= rect.y + insets.y, .width= rect.width - insets.x - insets.z, .height= rect.height - insets.y - insets.w };
// }

#define geom_INSET_POS_CENTER 0
#define geom_INSET_POS_SW 1
#define geom_INSET_POS_S 2
#define geom_INSET_POS_SE 3
#define geom_INSET_POS_W 4
#define geom_INSET_POS_E 5
#define geom_INSET_POS_NW 6
#define geom_INSET_POS_N 7
#define geom_INSET_POS_NE 8


static v4
geom_insets_rect_v4(v4 rect, v4 insets, s32 inset_pos)
{
	switch(inset_pos) {
	case geom_INSET_POS_CENTER: {
		return (v4) { .x = rect.x + insets.x, .y= rect.y + insets.y, .width= rect.width - insets.x - insets.z, .height= rect.height - insets.y - insets.w };
	};
	case geom_INSET_POS_S: {
		return (v4) { .x = rect.x + insets.x, .y= rect.y, .width= rect.width - insets.x - insets.z, .height = insets.y};
	};
	case geom_INSET_POS_W: {
		return (v4) { .x = rect.x, .y= rect.y + insets.y, .width= insets.x, .height= rect.height - insets.y - insets.w };
	};
	InvalidDefaultCase;
	}
	return (v4) { 0 };
}

internal v3
m4_transform_v3(m4 a, v3 p)
{
	return m4_transform_v4(a, (v4) { .xyz = p, .w = 1.0f }).xyz;
}

// typedef struct rectangle2i {
// 	s32 min_x, min_y;
// 	s32 max_x, max_y;
// } rect2i;

typedef struct {
	m4 forward;
	m4 inverse;
} m4i;

internal m4i
m4i_orthographic_projection(f32 aspect_ratio, f32 near_clip_plane, f32 far_clip_plane)
{
    f32 a = 1.0f;
    f32 b = aspect_ratio; // width over heigth

    f32 n = near_clip_plane; // NOTE(casey): Near clip plane _distance_
    f32 f = far_clip_plane;  // NOTE(casey): Far clip plane _distance_

    // NOTE(casey): These are the non-perspective corrected terms, for orthographic
    f32 d = 2.0f / (n - f);
    f32 e = (n + f) / (n - f);

    //
    // projection of (0, 0, -n, 1)
    // -n * d + e
    // = -n * 2/(n-f) + (n+f)/(n-f)
    // = (-2n + n + f) / (n-f)
    // = (f-n)/(n-f)
    // = -1
    //
    // projection of (0, 0, -f, 1)
    // -f * d + e
    // = -f * 2/(n-f) + (n+f)/(n-f)
    // = (-2f + n + f) / (n-f)
    // = (n-f)/(n-f)
    // = 1
    //

    return (m4i) {
	    .forward = (m4) {
		    .e = {
			    {a,  0,  0,  0},
			    {0,  b,  0,  0},
			    {0,  0,  d,  e},
			    {0,  0,  0,  1},
		    }
	    },
	    .inverse = (m4) {
		    .e = {
			    {1/a,   0,   0,    0},
			    {  0, 1/b,   0,    0},
			    {  0,   0, 1/d, -e/d},
			    {  0,   0,   0,    1},
		    }
	    }
    };

// #if HANDMADE_SLOW
//     m4x4 I = Result.Inverse*Result.Forward;
//     v3 Test0 = Result.Forward*V3(0, 0, -n);
//     v3 Test1 = Result.Forward*V3(0, 0, -f);
// #endif
}

internal m4i
m4i_perspective_projection(f32 aspect_ratio, f32 focal_length, f32 near_clip_plane, f32 far_clip_plane)
{
	f32 a = 1.0f;
	f32 b = aspect_ratio;
	f32 c = focal_length;

	f32 n = near_clip_plane; // NOTE(casey): Near clip plane _distance_
	f32 f = far_clip_plane; // NOTE(casey): Far clip plane _distance_

	// NOTE(casey): These are the perspective correct terms, for when you divide by -z
	f32 d = (n+f) / (n-f);
	f32 e = (2*f*n) / (n-f);

	m4i result = {
		.forward = (m4) {
			.e = {
				{a*c,    0,  0,  0},
				{  0,  b*c,  0,  0},
				{  0,    0,  d,  e},
				{  0,    0, -1,  0}
			}
		},
		.inverse = (m4) {
			.e = {
				{1/(a*c),       0,   0,   0},
				{      0, 1/(b*c),   0,   0},
				{      0,       0,   0,  -1},
				{      0,       0, 1/e, d/e}
			}
		}
	};
	//
	// #if HANDMADE_SLOW
	//     m4x4 I = Result.Inverse*Result.Forward;
	//     v4 Test0 = Result.Forward*V4(0, 0, -n, 1);
	//     Test0.xyz /= Test0.w;
	//     v4 Test1 = Result.Forward*V4(0, 0, -f, 1);
	//     Test1.xyz /= Test1.w;
	// #endif
	//
	return result;
}

internal m4i
m4i_camera_transform(v3 x, v3 y, v3 z, v3 p)
{
	m4 a = m4_from_rows(x, y, z);
	v3 minus_ap = v3_neg(m4_transform_v3(a,p));
	a = m4_translate(a, minus_ap);

	v3 ix = v3_inv_scale(x, v3_length_sq(x));
	v3 iy = v3_inv_scale(y, v3_length_sq(y));
	v3 iz = v3_inv_scale(z, v3_length_sq(z));
	v3 ip = (v3) {
		.x = minus_ap.x*ix.x + minus_ap.y*iy.x + minus_ap.z*iz.x,
		.y = minus_ap.x*ix.y + minus_ap.y*iy.y + minus_ap.z*iz.y,
		.z = minus_ap.x*ix.z + minus_ap.y*iy.z + minus_ap.z*iz.z
	};
	m4 b = m4_from_columns(ix, iy, iz);
	b = m4_translate(b, v3_neg(ip));

	return (m4i) { .forward = a, .inverse = b };

// #if HANDMADE_SLOW
// 	m4x4 I = Result.Inverse*Result.Forward;
// #endif
// 	return(Result);
}

//------------------------------------------------------------------------------
//
// ISO Spherical Coordinates
//
//------------------------------------------------------------------------------

//
// r radial distance
// theta = polar angle
// phi = azimuth_angle
//
static v3
geom_spherical_to_cartesian(v3 p)
{
	f32 cos_theta = pt_cos_f32(p.polar_angle);
	f32 sin_theta = pt_sin_f32(p.polar_angle);
	f32 cos_phi   = pt_cos_f32(p.azimuth_angle);
	f32 sin_phi   = pt_sin_f32(p.azimuth_angle);
	f32 x = p.radius * sin_theta * cos_phi;
	f32 y = p.radius * sin_theta * sin_phi;
	f32 z = p.radius * cos_theta;
	return (v3) { .x=x, .y=y, .z=z };
}

static v3
geom_cartesian_to_spherical(v3 p)
{
	f32 r     = v3_length(p);
	f32 theta = pt_acos_f32(p.z/r);
	f32 phi   = pt_atan2_f32(p.y,p.x);
	return (v3) { .radius=r, .polar_angle=theta, .azimuth_angle=phi };
}

static v3
geom_latlon_deg_to_spherical(f32 lat_deg, f32 lon_deg)
{
	f32 lon_rad = pt_deg_to_rad_f32(lon_deg);
	f32 lat_rad = pt_deg_to_rad_f32(lat_deg);
	return (v3) { .radius = 1.0f, .polar_angle = pt_PI - (lat_rad + (pt_PI / 2.0f)), .azimuth_angle = pt_PI + lon_rad };
}

static v3
geom_latlon_deg_to_cartesian(f32 lat_deg, f32 lon_deg)
{
	return geom_spherical_to_cartesian(geom_latlon_deg_to_spherical(lat_deg, lon_deg));
}

static v2
geom_latlon_deg_from_spherical(v3 p)
{
	// .polar_angle = pt_PI - (lat_rad + (pt_PI / 2.0f))
	// .polar_angle -pt_PI/2.0 = -lat_rad
	// lat_rad = -polar_angle + pt_PI/2.0
	f32 lon = pt_rad_to_deg_f32(p.azimuth_angle - pt_PI);
	f32 lat = pt_rad_to_deg_f32(-p.polar_angle + pt_PI/2.0f);
	return (v2) { .lat = lat, .lon = lon };
}

static v2
geom_latlon_deg_from_cartesian(v3 p)
{
	return geom_latlon_deg_from_spherical(geom_cartesian_to_spherical(p));
}

//
// intersect ray and sphere
//
// assuming ray_direction is normalized
//
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
//
static s32
geom_ray_sphere_intersection(v3 ray_start, v3 ray_direction, v3 sphere_center, f32 sphere_radius, v3 *intersection)
{
	v3  O = ray_start;
	v3  D = ray_direction;
	v3  C = sphere_center;
	f32 r = sphere_radius;
	f32 r_sqr = r * r;

	v3  L    = v3_sub(C, O);
	f32 t_ca = v3_inner(L, D);

	if (t_ca < 0) return 0;

	f32 d_sqr = v3_inner(L,L) - t_ca * t_ca;

	if (d_sqr > r_sqr) return 0;

	f32 t_hc = pt_sqrt_f32(r_sqr - d_sqr);

	f32 t0 = t_ca - t_hc;

	*intersection = v3_add(ray_start, v3_scale(ray_direction, t0));

	return 1; // (intersection, normal);
}

static v2
geom_latlon_to_mercator(v2 p)
{
	return (v2) {
		.mx = p.lon / 180.0,
		.my = pt_log_f32(pt_tan_f32((p.lat * pt_PI/180.0f)/2.0f + pt_PI / 4.0f)) / pt_PI
	};
}

static v2
geom_mercator_to_latlon(v2 p)
{
	return (v2) {
		.lon = p.mx * 180.0,
		.lat = (pt_atan_f32(pt_sinh_f32(p.my * pt_PI)) * 180.0f) / pt_PI
	};
}

// 2D tile

typedef union {
	struct {
		u64 x: 29;
		u64 y: 29;
		u64 level: 6;
	};
	u64 key;
} geom_Tile;

#define geom_TILE_MAX_LEVEL    29
#define geom_GEOHASH_MAX_LEVEL 32

static geom_Tile
geom_latlon_to_tile(v2 latlon, s32 level)
{
	Assert(level >=0 && level < 64);
	v2 mercator_coords = geom_latlon_to_mercator(latlon);
	s64 side = 1 << level;
	u64 tx = (u64) ((mercator_coords.x + 1.0)/2.0 * side);
	u64 ty = (u64) ((mercator_coords.y + 1.0)/2.0 * side);
	return (geom_Tile) { .x = tx, .y = ty, .level = level };
}

static u64
geom_geohash(f32 lat, f32 lon, s32 level)
{
	Assert(level >= 0 && level <= geom_GEOHASH_MAX_LEVEL);
	v2 mercator_coords = geom_latlon_to_mercator((v2) { .lat=lat, .lon=lon });

	s64 side = (s64) 1ull << level;
	u64 x = (u64) ((mercator_coords.x + 1.0)/2.0 * side);
	u64 y = (u64) ((mercator_coords.y + 1.0)/2.0 * side);

	u64 h    = 0;
	u64 mask = 1ull << (level-1);
	for (s32 i=0;i<level;++i) {
		h = (h << 2) + ((x & mask) ? 1 : 0) + ((y & mask) ? 2 : 0);
		mask = mask >> 1;
	}
	h = h << 2 * (geom_GEOHASH_MAX_LEVEL - level);
	return h;
}

static u64
geom_geohash_from_xy(u64 x, u64 y, s32 level)
{
	Assert(level >= 0 && level <= geom_GEOHASH_MAX_LEVEL);
	s64 side = (s64) 1ull << level;
	u64 h    = 0;
	u64 mask = 1ull << (level-1);
	for (s32 i=0;i<level;++i) {
		h = (h << 2) + ((x & mask) ? 1 : 0) + ((y & mask) ? 2 : 0);
		mask = mask >> 1;
	}
	h = h << 2 * (geom_GEOHASH_MAX_LEVEL - level);
	return h;
}

static u64
geom_geohash_from_xy_slippy(u64 x, u64 y, s32 level)
{
	Assert(level >= 0 && level <= geom_GEOHASH_MAX_LEVEL);
	s64 side = (s64) 1ull << level;
	y = side - 1 - y;
	u64 h    = 0;
	u64 mask = 1ull << (level-1);
	for (s32 i=0;i<level;++i) {
		h = (h << 2) + ((x & mask) ? 1 : 0) + ((y & mask) ? 2 : 0);
		mask = mask >> 1;
	}
	h = h << 2 * (geom_GEOHASH_MAX_LEVEL - level);
	return h;
}





static u64
geom_geohash_from_tile(geom_Tile tile)
{
	s32 level = tile.level; //_used = MIN(geom_TILE_MAX_LEVEL,tile.level);
	u64 h     = 0;
	s32 mask  = 1 << (level-1);
	for (s32 i=0;i<level;++i) {
		h = (h << 2) + ((tile.x & mask) ? 1 : 0) + ((tile.y & mask) ? 2 : 0);
		mask = mask >> 1;
	}
	h = h << 2 * (geom_GEOHASH_MAX_LEVEL - level);
	return h;
}

static geom_Tile
geom_geohash_to_tile(u64 geo_hash, s32 level)
{
	s32 x = 0;
	s32 y = 0;
	s32 shift = 2 * (geom_GEOHASH_MAX_LEVEL-1);
	for (s32 i=0;i<level;++i) {
		u64 val = geo_hash >> shift;
		x = 2*x + ((val & 1) ? 1 : 0);
		y = 2*y + ((val & 2) ? 1 : 0);
		shift -= 2;
	}
	return (geom_Tile) { .x=x, .y=y, .level=level };
}

static v4
geom_tile_corners_in_latlon(geom_Tile tile)
{
	// corners in mercator
	u64 cells_per_side = 1ull << tile.level;
	f64 mx0 = ((f64) tile.x / (f64) cells_per_side) * 2 - 1;
	f64 my0 = ((f64) tile.y / (f64) cells_per_side) * 2 - 1;
	f64 mx1 = ((f64) (tile.x + 1)/ (f64) cells_per_side) * 2 - 1;
	f64 my1 = ((f64) (tile.y + 1)/ (f64) cells_per_side) * 2 - 1;

	v2 a = geom_mercator_to_latlon((v2) { .mx=mx0, .my=my0 });
	v2 b = geom_mercator_to_latlon((v2) { .mx=mx1, .my=my1 });
	return (v4) { .lon0 = a.lon, .lat0 = a.lat, .lon1 = b.lon, .lat1 = b.lat };
}

//
// Moller and Trumbore
// http://www.lighthouse3d.com/tutorials/maths/ray-triangle-intersection/
//
#define geom_ray_triangle_intersection_EPSILON 1e-5
static s32
geom_ray_triangle_intersection(v3 p, v3 d, v3 v0, v3 v1, v3 v2, f32 *output_t)
{
	v3 e1 = v3_sub(v1,v0);
	v3 e2 = v3_sub(v2,v0);

	v3 h = v3_cross(d,e2);

	f32 a = v3_inner(e1,h);
	if (a > -geom_ray_triangle_intersection_EPSILON && a < geom_ray_triangle_intersection_EPSILON) {
		return 0;
	}

	// u,v are the barycentric coords

	f32 f = 1/a;
	v3 s = v3_sub(p,v0);
	f32 u = f * (v3_inner(s,h));

	if (u < 0.0 || u > 1.0)
		return(0);

	v3 q = v3_cross(s,e1);
	f32 v = f * v3_inner(d,q);

	if (v < 0.0 || u + v > 1.0)
		return(0);

	// at this stage we can compute t to find out where
	// the intersection point is on the line
	f32 t = f * v3_inner(e2,q);

	if (t > geom_ray_triangle_intersection_EPSILON) {
		*output_t = t;
		return(1); // ray intersection
	} else {
		// this means that there is a line intersection
		// but not a ray intersection
		return (0);
	}
}

#define geom_ray_plane_intersection_EPSILON 1e-8
static s32
geom_ray_plane_intersection(v3 ray_origin, v3 ray, v3 plane, f32 *output_t)
{
	f32 denominator  = ray.x*plane.x + ray.y*plane.y + ray.z*plane.z;
	f32 numerator    = ray_origin.x*plane.x + ray_origin.y*plane.y + ray_origin.z*plane.z;
	if (denominator < -geom_ray_plane_intersection_EPSILON || denominator > geom_ray_plane_intersection_EPSILON) {
		*output_t = -numerator / denominator;
		return 1;
	} else {
		return 0;
	}
}




