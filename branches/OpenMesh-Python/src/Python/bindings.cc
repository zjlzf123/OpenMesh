#include <Python.h>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/python.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/copy_const_reference.hpp>
#include "OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh"
#include "OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh"
#include "OpenMesh/Core/Utils/PropertyManager.hh"

using namespace boost::python;

struct MeshTraits : public OpenMesh::DefaultTraits
{
	/// Use double precision points
	typedef OpenMesh::Vec3d Point;

	/// Use double precision Normals
	typedef OpenMesh::Vec3d Normal;

	/// Use RGBA Color
	typedef OpenMesh::Vec4f Color;
};

typedef OpenMesh::TriMesh_ArrayKernelT<MeshTraits> TriMesh;
typedef OpenMesh::PolyMesh_ArrayKernelT<MeshTraits> PolyMesh;

typedef OpenMesh::BaseHandle BaseHandle;
typedef OpenMesh::VertexHandle VertexHandle;
typedef OpenMesh::HalfedgeHandle HalfedgeHandle;
typedef OpenMesh::EdgeHandle EdgeHandle;
typedef OpenMesh::FaceHandle FaceHandle;

template <class Vector, class Scalar>
void set_item(Vector& _vec, int _index, Scalar _value) {
	if (_index < 0) {
		_index += _vec.size();
	}

	if ((size_t)_index < _vec.size()) {
		_vec[_index] = _value;
	}
	else {
		PyErr_SetString(PyExc_IndexError, "index out of range");
		throw_error_already_set();
	}
}

template <class Vector, class Scalar>
Scalar get_item(Vector& _vec, int _index) {
	if (_index < 0) {
		_index += _vec.size();
	}

	if ((size_t)_index < _vec.size()) {
		return _vec[_index];
	}
	else {
		PyErr_SetString(PyExc_IndexError, "index out of range");
		throw_error_already_set();
	}

	return 0.0;
}

template<class Iterator, size_t (OpenMesh::ArrayKernel::*n_items)() const>
class IteratorWrapperT {
	public:
		IteratorWrapperT(const OpenMesh::PolyConnectivity& _mesh) :
			mesh_(_mesh), n_items_(n_items),
			iterator_(_mesh, typename Iterator::value_type(0)),
			iterator_end_(_mesh, typename Iterator::value_type(int((_mesh.*n_items)()))) {
		}

		IteratorWrapperT iter() const {
			return *this;
		}

		typename Iterator::value_type next() {
			if (iterator_ != iterator_end_) {
				typename Iterator::value_type res = *iterator_;
				++iterator_;
				return res;
			}
			else {
				PyErr_SetString(PyExc_StopIteration, "No more data.");
				throw_error_already_set();
			}
			return typename Iterator::value_type();
		}

		unsigned int len() const {
			return (mesh_.*n_items_)();
		}

	private:
		const OpenMesh::PolyConnectivity& mesh_;
		size_t (OpenMesh::ArrayKernel::*n_items_)() const;
		Iterator iterator_;
		Iterator iterator_end_;
};

template<class Circulator>
class CirculatorWrapperT {
	public:
		CirculatorWrapperT(const typename Circulator::mesh_type& _mesh, typename Circulator::center_type _center) :
			circulator_(_mesh, _center) {
		}

		CirculatorWrapperT iter() const {
			return *this;
		}

		typename Circulator::value_type next() {
			if (circulator_.is_valid()) {
				typename Circulator::value_type res = *circulator_;
				++circulator_;
				return res;
			}
			else {
				PyErr_SetString(PyExc_StopIteration, "No more data.");
				throw_error_already_set();
			}
			return typename Circulator::value_type();
		}

	private:
		Circulator circulator_;
};

template<class Mesh>
class MeshWrapperT : public Mesh {
	public:
		IteratorWrapperT<OpenMesh::PolyConnectivity::VertexIter, &OpenMesh::ArrayKernel::n_vertices> vertices() const {
			return IteratorWrapperT<OpenMesh::PolyConnectivity::VertexIter, &OpenMesh::ArrayKernel::n_vertices>(*this);
		}

		IteratorWrapperT<OpenMesh::PolyConnectivity::HalfedgeIter, &OpenMesh::ArrayKernel::n_halfedges> halfedges() const {
			return IteratorWrapperT<OpenMesh::PolyConnectivity::HalfedgeIter, &OpenMesh::ArrayKernel::n_halfedges>(*this);
		}

		IteratorWrapperT<OpenMesh::PolyConnectivity::EdgeIter, &OpenMesh::ArrayKernel::n_edges> edges() const {
			return IteratorWrapperT<OpenMesh::PolyConnectivity::EdgeIter, &OpenMesh::ArrayKernel::n_edges>(*this);
		}

		IteratorWrapperT<OpenMesh::PolyConnectivity::FaceIter, &OpenMesh::ArrayKernel::n_faces> faces() const {
			return IteratorWrapperT<OpenMesh::PolyConnectivity::FaceIter, &OpenMesh::ArrayKernel::n_faces>(*this);
		}

		CirculatorWrapperT<typename Mesh::VertexVertexIter> vv(VertexHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::VertexVertexIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::VertexIHalfedgeIter> vih(VertexHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::VertexIHalfedgeIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::VertexOHalfedgeIter> voh(VertexHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::VertexOHalfedgeIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::VertexEdgeIter> ve(VertexHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::VertexEdgeIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::VertexFaceIter> vf(VertexHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::VertexFaceIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::FaceVertexIter> fv(FaceHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::FaceVertexIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::FaceHalfedgeIter> fh(FaceHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::FaceHalfedgeIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::FaceEdgeIter> fe(FaceHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::FaceEdgeIter>(*this, _handle);
		}

		CirculatorWrapperT<typename Mesh::FaceFaceIter> ff(FaceHandle _handle) const {
			return CirculatorWrapperT<typename Mesh::FaceFaceIter>(*this, _handle);
		}

		void garbage_collection() {
			Mesh::garbage_collection();
		}

		void set_status(VertexHandle _vh, const OpenMesh::Attributes::StatusInfo &_info) {
			Mesh::status(_vh) = _info;
		}

		void set_status(HalfedgeHandle _heh, const OpenMesh::Attributes::StatusInfo &_info) {
			Mesh::status(_heh) = _info;
		}

		void set_status(EdgeHandle _eh, const OpenMesh::Attributes::StatusInfo &_info) {
			Mesh::status(_eh) = _info;
		}

		void set_status(FaceHandle _fh, const OpenMesh::Attributes::StatusInfo &_info) {
			Mesh::status(_fh) = _info;
		}
};

template<class PropertyManager, class IndexHandle>
class PropertyManagerWrapperT : public PropertyManager {
	public:
		PropertyManagerWrapperT(OpenMesh::PolyConnectivity &_mesh, const char *_propname, bool _existing = false) :
			PropertyManager(_mesh, _propname, _existing) {
		}

		PropertyManagerWrapperT() : PropertyManager() { }

		object getitem(IndexHandle _handle) const {
			return (*this)[_handle];
		}

		void setitem(IndexHandle _handle, object _item) {
			(*this)[_handle] = _item;
		}
};

template<class Scalar, int N>
void expose_vec(const char *_name) {
	typedef OpenMesh::VectorT<Scalar, N> Vector;

	Scalar (Vector::*min_void)() const = &Vector::min;
	Scalar (Vector::*max_void)() const = &Vector::max;

	Vector (Vector::*max_vector)(const Vector&) const = &Vector::max;
	Vector (Vector::*min_vector)(const Vector&) const = &Vector::min;

	class_<Vector> classVector = class_<Vector>(_name);

	classVector
		.def("__setitem__", &set_item<Vector, Scalar>)
		.def("__getitem__", &get_item<Vector, Scalar>)
		.def(self == self)
		.def(self != self)
		.def(self *= Scalar())
		.def(self /= Scalar())
		.def(self * Scalar())
		.def(Scalar() * self)
		.def(self / Scalar())
		.def(self *= self)
		.def(self /= self)
		.def(self -= self)
		.def(self += self)
		.def(self * self)
		.def(self / self)
		.def(self + self)
		.def(self - self)
		.def(-self)
		.def("dot", &Vector::operator|)
		.def("vectorize", &Vector::vectorize, return_internal_reference<>())
		.def(self < self)

		.def("norm", &Vector::norm)
		.def("length", &Vector::length)
		.def("sqrnorm", &Vector::sqrnorm)
		.def("normalize", &Vector::normalize, return_internal_reference<>())
		.def("normalized", &Vector::normalized)
		.def("normalize_cond", &Vector::normalize_cond, return_internal_reference<>())

		.def("l1_norm", &Vector::l1_norm)
		.def("l8_norm", &Vector::l8_norm)

		.def("max", max_void)
		.def("max_abs", &Vector::max_abs)
		.def("min", min_void)
		.def("min_abs", &Vector::min_abs)
		.def("mean", &Vector::mean)
		.def("mean_abs", &Vector::mean_abs)
		.def("minimize", &Vector::minimize, return_internal_reference<>())
		.def("minimized", &Vector::minimized)
		.def("maximize", &Vector::maximize, return_internal_reference<>())
		.def("maximized", &Vector::maximized)
		.def("min", min_vector)
		.def("max", max_vector)

		.def("size", &Vector::size)
		.staticmethod("size")
		.def("vectorized", &Vector::vectorized)
		.staticmethod("vectorized")
		;

	typedef OpenMesh::VectorT<Scalar, 2> Vector2;
	typedef OpenMesh::VectorT<Scalar, 3> Vector3;
	typedef OpenMesh::VectorT<Scalar, 4> Vector4;

	struct Factory {
		static Vector2 *vec2_default() {
			return new Vector2(Scalar(), Scalar());
		}
		static Vector2 *vec2_user_defined(const Scalar& _v0, const Scalar& _v1) {
			return new Vector2(_v0, _v1);
		}
		static Vector3 *vec3_default() {
			return new Vector3(Scalar(), Scalar(), Scalar());
		}
		static Vector3 *vec3_user_defined(const Scalar& _v0, const Scalar& _v1, const Scalar& _v2) {
			return new Vector3(_v0, _v1, _v2);
		}
		static Vector4 *vec4_default() {
			return new Vector4(Scalar(), Scalar(), Scalar(), Scalar());
		}
		static Vector4 *vec4_user_defined(const Scalar& _v0, const Scalar& _v1, const Scalar& _v2, const Scalar& _v3) {
			return new Vector4(_v0, _v1, _v2, _v3);
		}
	};

	Vector3 (Vector3::*cross)(const Vector3&) const = &Vector3::operator%;

	if (N == 2) {
		classVector
			.def("__init__", make_constructor(&Factory::vec2_default))
			.def("__init__", make_constructor(&Factory::vec2_user_defined))
			;
	}

	if (N == 3) {
		classVector
			.def("__init__", make_constructor(&Factory::vec3_default))
			.def("__init__", make_constructor(&Factory::vec3_user_defined))
			.def("cross", cross)
			;
	}

	if (N == 4) {
		classVector
			.def("__init__", make_constructor(&Factory::vec4_default))
			.def("__init__", make_constructor(&Factory::vec4_user_defined))
			;
	}
}

void expose_handles() {
	class_<BaseHandle>("BaseHandle")
		.def("idx", &BaseHandle::idx)
		.def("is_valid", &BaseHandle::is_valid)
		.def("reset", &BaseHandle::reset)
		.def("invalidate", &BaseHandle::invalidate)
		.def(self == self)
		.def(self != self)
		.def(self < self)
		;

	class_<VertexHandle, bases<BaseHandle> >("VertexHandle");
	class_<HalfedgeHandle, bases<BaseHandle> >("HalfedgeHandle");
	class_<EdgeHandle, bases<BaseHandle> >("EdgeHandle");
	class_<FaceHandle, bases<BaseHandle> >("FaceHandle");
}

void expose_status_bits_and_info() {
	using OpenMesh::Attributes::StatusBits;
	using OpenMesh::Attributes::StatusInfo;

	enum_<StatusBits>("StatusBits")
		.value("DELETED", OpenMesh::Attributes::DELETED)
		.value("LOCKED", OpenMesh::Attributes::LOCKED)
		.value("SELECTED", OpenMesh::Attributes::SELECTED)
		.value("HIDDEN", OpenMesh::Attributes::HIDDEN)
		.value("FEATURE", OpenMesh::Attributes::FEATURE)
		.value("TAGGED", OpenMesh::Attributes::TAGGED)
		.value("TAGGED2", OpenMesh::Attributes::TAGGED2)
		.value("FIXEDNONMANIFOLD", OpenMesh::Attributes::FIXEDNONMANIFOLD)
		.value("UNUSED", OpenMesh::Attributes::UNUSED)
		;

	class_<StatusInfo>("StatusInfo")
		.def("deleted", &StatusInfo::deleted)
		.def("set_deleted", &StatusInfo::set_deleted)
		.def("locked", &StatusInfo::locked)
		.def("set_locked", &StatusInfo::set_locked)
		.def("selected", &StatusInfo::selected)
		.def("set_selected", &StatusInfo::set_selected)
		.def("hidden", &StatusInfo::hidden)
		.def("set_hidden", &StatusInfo::set_hidden)
		.def("feature", &StatusInfo::feature)
		.def("set_feature", &StatusInfo::set_feature)
		.def("tagged", &StatusInfo::tagged)
		.def("set_tagged", &StatusInfo::set_tagged)
		.def("tagged2", &StatusInfo::tagged2)
		.def("set_tagged2", &StatusInfo::set_tagged2)
		.def("fixed_nonmanifold", &StatusInfo::fixed_nonmanifold)
		.def("set_fixed_nonmanifold", &StatusInfo::set_fixed_nonmanifold)
		.def("bits", &StatusInfo::bits)
		.def("set_bits", &StatusInfo::set_bits)
		.def("is_bit_set", &StatusInfo::is_bit_set)
		.def("set_bit", &StatusInfo::set_bit)
		.def("unset_bit", &StatusInfo::unset_bit)
		.def("change_bit", &StatusInfo::change_bit)
		;
}

template<class Mesh>
void expose_mesh(const char *_name) {
	using OpenMesh::Attributes::StatusInfo;

	VertexHandle   (Mesh::*vertex_handle_uint  )(unsigned int  ) const = &Mesh::vertex_handle;
	HalfedgeHandle (Mesh::*halfedge_handle_uint)(unsigned int  ) const = &Mesh::halfedge_handle;
	EdgeHandle     (Mesh::*edge_handle_uint    )(unsigned int  ) const = &Mesh::edge_handle;
	FaceHandle     (Mesh::*face_handle_uint    )(unsigned int  ) const = &Mesh::face_handle;
	HalfedgeHandle (Mesh::*halfedge_handle_vh  )(VertexHandle  ) const = &Mesh::halfedge_handle;
	HalfedgeHandle (Mesh::*halfedge_handle_fh  )(FaceHandle    ) const = &Mesh::halfedge_handle;

	EdgeHandle     (Mesh::*edge_handle_heh     )(HalfedgeHandle) const = &Mesh::edge_handle;
	FaceHandle     (Mesh::*face_handle_heh     )(HalfedgeHandle) const = &Mesh::face_handle;

	HalfedgeHandle (Mesh::*halfedge_handle_eh_uint)(EdgeHandle, unsigned int) const = &Mesh::halfedge_handle;
	HalfedgeHandle (Mesh::*prev_halfedge_handle_heh)(HalfedgeHandle) const = &Mesh::prev_halfedge_handle;

	void (Mesh::*set_halfedge_handle_vh_heh)(VertexHandle, HalfedgeHandle) = &Mesh::set_halfedge_handle;
	void (Mesh::*set_halfedge_handle_fh_heh)(FaceHandle, HalfedgeHandle  ) = &Mesh::set_halfedge_handle;

	FaceHandle (Mesh::*add_face)(VertexHandle, VertexHandle, VertexHandle) = &Mesh::add_face;

	// Get value of a standard property (point, normal, color)
	const typename Mesh::Point&  (Mesh::*point_vh )(VertexHandle  ) const = &Mesh::point;
	const typename Mesh::Normal& (Mesh::*normal_vh)(VertexHandle  ) const = &Mesh::normal;
	const typename Mesh::Normal& (Mesh::*normal_hh)(HalfedgeHandle) const = &Mesh::normal;
	const typename Mesh::Normal& (Mesh::*normal_fh)(FaceHandle    ) const = &Mesh::normal;
	const typename Mesh::Color&  (Mesh::*color_vh )(VertexHandle  ) const = &Mesh::color;
	const typename Mesh::Color&  (Mesh::*color_hh )(HalfedgeHandle) const = &Mesh::color;
	const typename Mesh::Color&  (Mesh::*color_eh )(EdgeHandle    ) const = &Mesh::color;
	const typename Mesh::Color&  (Mesh::*color_fh )(FaceHandle    ) const = &Mesh::color;

	// Get value of a standard property (texture coordinate)
	const typename Mesh::TexCoord1D& (Mesh::*texcoord1D_vh)(VertexHandle  ) const = &Mesh::texcoord1D;
	const typename Mesh::TexCoord1D& (Mesh::*texcoord1D_hh)(HalfedgeHandle) const = &Mesh::texcoord1D;
	const typename Mesh::TexCoord2D& (Mesh::*texcoord2D_vh)(VertexHandle  ) const = &Mesh::texcoord2D;
	const typename Mesh::TexCoord2D& (Mesh::*texcoord2D_hh)(HalfedgeHandle) const = &Mesh::texcoord2D;
	const typename Mesh::TexCoord3D& (Mesh::*texcoord3D_vh)(VertexHandle  ) const = &Mesh::texcoord3D;
	const typename Mesh::TexCoord3D& (Mesh::*texcoord3D_hh)(HalfedgeHandle) const = &Mesh::texcoord3D;

	// Get value of a standard property (status)
	const StatusInfo& (Mesh::*status_vh)(VertexHandle  ) const = &Mesh::status;
	const StatusInfo& (Mesh::*status_hh)(HalfedgeHandle) const = &Mesh::status;
	const StatusInfo& (Mesh::*status_eh)(EdgeHandle    ) const = &Mesh::status;
	const StatusInfo& (Mesh::*status_fh)(FaceHandle    ) const = &Mesh::status;

	// Set value of a standard property (point, normal, color)
	void (Mesh::*set_normal_vh)(VertexHandle,   const typename Mesh::Normal&) = &Mesh::set_normal;
	void (Mesh::*set_normal_hh)(HalfedgeHandle, const typename Mesh::Normal&) = &Mesh::set_normal;
	void (Mesh::*set_normal_fh)(FaceHandle,     const typename Mesh::Normal&) = &Mesh::set_normal;
	void (Mesh::*set_color_vh )(VertexHandle,   const typename Mesh::Color& ) = &Mesh::set_color;
	void (Mesh::*set_color_hh )(HalfedgeHandle, const typename Mesh::Color& ) = &Mesh::set_color;
	void (Mesh::*set_color_eh )(EdgeHandle,     const typename Mesh::Color& ) = &Mesh::set_color;
	void (Mesh::*set_color_fh )(FaceHandle,     const typename Mesh::Color& ) = &Mesh::set_color;

	// Set value of a standard property (texture coordinate)
	void (Mesh::*set_texcoord1D_vh)(VertexHandle,   const typename Mesh::TexCoord1D&) = &Mesh::set_texcoord1D;
	void (Mesh::*set_texcoord1D_hh)(HalfedgeHandle, const typename Mesh::TexCoord1D&) = &Mesh::set_texcoord1D;
	void (Mesh::*set_texcoord2D_vh)(VertexHandle,   const typename Mesh::TexCoord2D&) = &Mesh::set_texcoord2D;
	void (Mesh::*set_texcoord2D_hh)(HalfedgeHandle, const typename Mesh::TexCoord2D&) = &Mesh::set_texcoord2D;
	void (Mesh::*set_texcoord3D_vh)(VertexHandle,   const typename Mesh::TexCoord3D&) = &Mesh::set_texcoord3D;
	void (Mesh::*set_texcoord3D_hh)(HalfedgeHandle, const typename Mesh::TexCoord3D&) = &Mesh::set_texcoord3D;

	// Set value of a standard property (status)
	void (Mesh::*set_status_vh)(VertexHandle,   const StatusInfo&) = &Mesh::set_status;
	void (Mesh::*set_status_hh)(HalfedgeHandle, const StatusInfo&) = &Mesh::set_status;
	void (Mesh::*set_status_eh)(EdgeHandle,     const StatusInfo&) = &Mesh::set_status;
	void (Mesh::*set_status_fh)(FaceHandle,     const StatusInfo&) = &Mesh::set_status;

	class_<Mesh> classMesh = class_<Mesh>(_name);

	/*
	 * It is important that we enter the scope before we add
	 * the definitions because in some of the builders classes
	 * which are supposed to be inside the scope are defined.
	 */
	scope scope_Mesh = classMesh;

	classMesh
		.def("vertex_handle", vertex_handle_uint)
		.def("halfedge_handle", halfedge_handle_uint)
		.def("edge_handle", edge_handle_uint)
		.def("face_handle", face_handle_uint)

		.def("clear", &Mesh::clear)
		.def("clean", &Mesh::clean)
		.def("garbage_collection", &Mesh::garbage_collection)

		.def("n_vertices", &Mesh::n_vertices)
		.def("n_halfedges", &Mesh::n_halfedges)
		.def("n_edges", &Mesh::n_edges)
		.def("n_faces", &Mesh::n_faces)
		.def("vertices_empty", &Mesh::vertices_empty)
		.def("halfedges_empty", &Mesh::halfedges_empty)
		.def("edges_empty", &Mesh::edges_empty)
		.def("faces_empty", &Mesh::faces_empty)

		.def("halfedge_handle", halfedge_handle_vh)
		.def("set_halfedge_handle", set_halfedge_handle_vh_heh)

		.def("to_vertex_handle", &Mesh::to_vertex_handle)
		.def("from_vertex_handle", &Mesh::from_vertex_handle)
		.def("set_vertex_handle", &Mesh::set_vertex_handle)
		.def("face_handle", face_handle_heh)
		.def("set_face_handle", &Mesh::set_face_handle)
		.def("next_halfedge_handle", &Mesh::next_halfedge_handle)
		.def("set_next_halfedge_handle", &Mesh::set_next_halfedge_handle)
		.def("prev_halfedge_handle", prev_halfedge_handle_heh)
		.def("opposite_halfedge_handle", &Mesh::opposite_halfedge_handle)
		.def("ccw_rotated_halfedge_handle", &Mesh::ccw_rotated_halfedge_handle)
		.def("cw_rotated_halfedge_handle", &Mesh::cw_rotated_halfedge_handle)
		.def("edge_handle", edge_handle_heh)

		.def("halfedge_handle", halfedge_handle_eh_uint)

		.def("halfedge_handle", halfedge_handle_fh)
		.def("set_halfedge_handle", set_halfedge_handle_fh_heh)

		.def("point", point_vh, return_value_policy<copy_const_reference>())
		.def("set_point", &Mesh::set_point)
		.def("normal", normal_vh, return_value_policy<copy_const_reference>())
		.def("set_normal", set_normal_vh)
		.def("normal", normal_hh, return_value_policy<copy_const_reference>())
		.def("set_normal", set_normal_hh)
		.def("color", color_vh, return_value_policy<copy_const_reference>())
		.def("set_color", set_color_vh)
		.def("texcoord1D", texcoord1D_vh, return_value_policy<copy_const_reference>())
		.def("set_texcoord1D", set_texcoord1D_vh)
		.def("texcoord2D", texcoord2D_vh, return_value_policy<copy_const_reference>())
		.def("set_texcoord2D", set_texcoord2D_vh)
		.def("texcoord3D", texcoord3D_vh, return_value_policy<copy_const_reference>())
		.def("set_texcoord3D", set_texcoord3D_vh)
		.def("texcoord1D", texcoord1D_hh, return_value_policy<copy_const_reference>())
		.def("set_texcoord1D", set_texcoord1D_hh)
		.def("texcoord2D", texcoord2D_hh, return_value_policy<copy_const_reference>())
		.def("set_texcoord2D", set_texcoord2D_hh)
		.def("texcoord3D", texcoord3D_hh, return_value_policy<copy_const_reference>())
		.def("set_texcoord3D", set_texcoord3D_hh)
		.def("status", status_vh, return_value_policy<copy_const_reference>())
		.def("set_status", set_status_vh)
		.def("status", status_hh, return_value_policy<copy_const_reference>())
		.def("set_status", set_status_hh)
		.def("color", color_hh, return_value_policy<copy_const_reference>())
		.def("set_color", set_color_hh)
		.def("color", color_eh, return_value_policy<copy_const_reference>())
		.def("set_color", set_color_eh)
		.def("status", status_eh, return_value_policy<copy_const_reference>())
		.def("set_status", set_status_eh)
		.def("normal", normal_fh, return_value_policy<copy_const_reference>())
		.def("set_normal", set_normal_fh)
		.def("color", color_fh, return_value_policy<copy_const_reference>())
		.def("set_color", set_color_fh)
		.def("status", status_fh, return_value_policy<copy_const_reference>())
		.def("set_status", set_status_fh)

		.def("request_vertex_normals", &Mesh::request_vertex_normals)
		.def("request_vertex_colors", &Mesh::request_vertex_colors)
		.def("request_vertex_texcoords1D", &Mesh::request_vertex_texcoords1D)
		.def("request_vertex_texcoords2D", &Mesh::request_vertex_texcoords2D)
		.def("request_vertex_texcoords3D", &Mesh::request_vertex_texcoords3D)
		.def("request_vertex_status", &Mesh::request_vertex_status)
		.def("request_halfedge_status", &Mesh::request_halfedge_status)
		.def("request_halfedge_normals", &Mesh::request_halfedge_normals)
		.def("request_halfedge_colors", &Mesh::request_halfedge_colors)
		.def("request_halfedge_texcoords1D", &Mesh::request_halfedge_texcoords1D)
		.def("request_halfedge_texcoords2D", &Mesh::request_halfedge_texcoords2D)
		.def("request_halfedge_texcoords3D", &Mesh::request_halfedge_texcoords3D)
		.def("request_edge_status", &Mesh::request_edge_status)
		.def("request_edge_colors", &Mesh::request_edge_colors)
		.def("request_face_normals", &Mesh::request_face_normals)
		.def("request_face_colors", &Mesh::request_face_colors)
		.def("request_face_status", &Mesh::request_face_status)
		.def("request_face_texture_index", &Mesh::request_face_texture_index)

		.def("release_vertex_normals", &Mesh::release_vertex_normals)
		.def("release_vertex_colors", &Mesh::release_vertex_colors)
		.def("release_vertex_texcoords1D", &Mesh::release_vertex_texcoords1D)
		.def("release_vertex_texcoords2D", &Mesh::release_vertex_texcoords2D)
		.def("release_vertex_texcoords3D", &Mesh::release_vertex_texcoords3D)
		.def("release_vertex_status", &Mesh::release_vertex_status)
		.def("release_halfedge_status", &Mesh::release_halfedge_status)
		.def("release_halfedge_normals", &Mesh::release_halfedge_normals)
		.def("release_halfedge_colors", &Mesh::release_halfedge_colors)
		.def("release_halfedge_texcoords1D", &Mesh::release_halfedge_texcoords1D)
		.def("release_halfedge_texcoords2D", &Mesh::release_halfedge_texcoords2D)
		.def("release_halfedge_texcoords3D", &Mesh::release_halfedge_texcoords3D)
		.def("release_edge_status", &Mesh::release_edge_status)
		.def("release_edge_colors", &Mesh::release_edge_colors)
		.def("release_face_normals", &Mesh::release_face_normals)
		.def("release_face_colors", &Mesh::release_face_colors)
		.def("release_face_status", &Mesh::release_face_status)
		.def("release_face_texture_index", &Mesh::release_face_texture_index)

		.def("has_vertex_normals", &Mesh::has_vertex_normals)
		.def("has_vertex_colors", &Mesh::has_vertex_colors)
		.def("has_vertex_texcoords1D", &Mesh::has_vertex_texcoords1D)
		.def("has_vertex_texcoords2D", &Mesh::has_vertex_texcoords2D)
		.def("has_vertex_texcoords3D", &Mesh::has_vertex_texcoords3D)
		.def("has_vertex_status", &Mesh::has_vertex_status)
		.def("has_halfedge_status", &Mesh::has_halfedge_status)
		.def("has_halfedge_normals", &Mesh::has_halfedge_normals)
		.def("has_halfedge_colors", &Mesh::has_halfedge_colors)
		.def("has_halfedge_texcoords1D", &Mesh::has_halfedge_texcoords1D)
		.def("has_halfedge_texcoords2D", &Mesh::has_halfedge_texcoords2D)
		.def("has_halfedge_texcoords3D", &Mesh::has_halfedge_texcoords3D)
		.def("has_edge_status", &Mesh::has_edge_status)
		.def("has_edge_colors", &Mesh::has_edge_colors)
		.def("has_face_normals", &Mesh::has_face_normals)
		.def("has_face_colors", &Mesh::has_face_colors)
		.def("has_face_status", &Mesh::has_face_status)
		.def("has_face_texture_index", &Mesh::has_face_texture_index)

		.def("vertices", &Mesh::vertices)
		.def("halfedges", &Mesh::halfedges)
		.def("edges", &Mesh::edges)
		.def("faces", &Mesh::faces)

		.def("vv", &Mesh::vv)
		.def("vih", &Mesh::vih)
		.def("voh", &Mesh::voh)
		.def("ve", &Mesh::ve)
		.def("vf", &Mesh::vf)

		.def("fv", &Mesh::fv)
		.def("fh", &Mesh::fh)
		.def("fe", &Mesh::fe)
		.def("ff", &Mesh::ff)

		.def("add_vertex", &Mesh::add_vertex)
		.def("add_face", add_face)
		.def("vertex_handle", &Mesh::vertex_handle)
		;
}

template<class Iterator, size_t (OpenMesh::ArrayKernel::*n_items)() const>
void expose_iterator(const char *_name) {
	class_<IteratorWrapperT<Iterator, n_items> >(_name, init<MeshWrapperT<TriMesh>&>())
		.def(init<MeshWrapperT<PolyMesh>&>())
		.def("__iter__", &IteratorWrapperT<Iterator, n_items>::iter)
		.def("__next__", &IteratorWrapperT<Iterator, n_items>::next)
		.def("__len__", &IteratorWrapperT<Iterator, n_items>::len)
		.def("next", &IteratorWrapperT<Iterator, n_items>::next)
		;
}

template<class Circulator>
void expose_circulator(const char *_name) {
	class_<CirculatorWrapperT<Circulator> >(_name, init<MeshWrapperT<TriMesh>&, typename Circulator::center_type>())
		.def(init<MeshWrapperT<PolyMesh>&, typename Circulator::center_type>())
		.def("__iter__", &CirculatorWrapperT<Circulator>::iter)
		.def("__next__", &CirculatorWrapperT<Circulator>::next)
		.def("next", &CirculatorWrapperT<Circulator>::next)
		;
}

template<class PropHandle, class IndexHandle>
void expose_property_manager(const char *_name) {
	typedef OpenMesh::PropertyManager<PropHandle, OpenMesh::PolyConnectivity > PropertyManager;
	typedef PropertyManagerWrapperT<PropertyManager, IndexHandle> PropertyManagerWrapper;

	class_<PropertyManagerWrapper, boost::noncopyable>(_name)
		.def(init<MeshWrapperT<TriMesh>&, const char *, optional<bool> >())
		.def(init<MeshWrapperT<PolyMesh>&, const char *, optional<bool> >())
		.def("__getitem__", &PropertyManagerWrapper::getitem)
		.def("__setitem__", &PropertyManagerWrapper::setitem)
		;
}

BOOST_PYTHON_MODULE(openmesh) {
	expose_vec<float,  2>("Vec2f");
	expose_vec<float,  3>("Vec3f");
	expose_vec<float,  4>("Vec4f");
	expose_vec<double, 2>("Vec2d");
	expose_vec<double, 3>("Vec3d");
	expose_vec<double, 4>("Vec4d");

	expose_handles();
	expose_status_bits_and_info();

	expose_mesh<MeshWrapperT<TriMesh> >("TriMesh");
	expose_mesh<MeshWrapperT<PolyMesh> >("PolyMesh");

	expose_iterator<OpenMesh::PolyConnectivity::VertexIter, &OpenMesh::ArrayKernel::n_vertices>("VertexIter");
	expose_iterator<OpenMesh::PolyConnectivity::HalfedgeIter, &OpenMesh::ArrayKernel::n_halfedges>("HalfedgeIter");
	expose_iterator<OpenMesh::PolyConnectivity::EdgeIter, &OpenMesh::ArrayKernel::n_edges>("EdgeIter");
	expose_iterator<OpenMesh::PolyConnectivity::FaceIter, &OpenMesh::ArrayKernel::n_faces>("FaceIter");

	expose_circulator<OpenMesh::PolyConnectivity::VertexVertexIter>("VertexVertexIter");
	expose_circulator<OpenMesh::PolyConnectivity::VertexIHalfedgeIter>("VertexIHalfedgeIter");
	expose_circulator<OpenMesh::PolyConnectivity::VertexOHalfedgeIter>("VertexOHalfedgeIter");
	expose_circulator<OpenMesh::PolyConnectivity::VertexEdgeIter>("VertexEdgeIter");
	expose_circulator<OpenMesh::PolyConnectivity::VertexFaceIter>("VertexFaceIter");

	expose_circulator<OpenMesh::PolyConnectivity::FaceVertexIter>("FaceVertexIter");
	expose_circulator<OpenMesh::PolyConnectivity::FaceHalfedgeIter>("FaceHalfedgeIter");
	expose_circulator<OpenMesh::PolyConnectivity::FaceEdgeIter>("FaceEdgeIter");
	expose_circulator<OpenMesh::PolyConnectivity::FaceFaceIter>("FaceFaceIter");

	// TODO expose_circulator<OpenMesh::PolyConnectivity::HalfedgeLoopIter>("HalfedgeLoopIter");

	expose_property_manager<OpenMesh::VPropHandleT<object>, VertexHandle>("VPropertyManager");
}