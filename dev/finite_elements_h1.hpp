// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443211. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the MFEM library. For more information and source code
// availability see http://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#ifndef MFEM_TEMPLATE_FINITE_ELEMENTS_H1
#define MFEM_TEMPLATE_FINITE_ELEMENTS_H1

#include "config.hpp"
#include "fem/fe_coll.hpp"

namespace mfem
{

// H1 finite elements

template <typename real_t>
void CalcShapeMatrix(const FiniteElement &fe, const IntegrationRule &ir,
                     real_t *B, const Array<int> *dof_map = NULL)
{
   // - B must be (nip x dof) with column major storage
   // - The inverse of dof_map is applied to reorder the local dofs.
   int nip = ir.GetNPoints();
   int dof = fe.GetDof();
   Vector shape(dof);

   for (int ip = 0; ip < nip; ip++)
   {
      fe.CalcShape(ir.IntPoint(ip), shape);
      for (int id = 0; id < dof; id++)
      {
         int orig_id = dof_map ? (*dof_map)[id] : id;
         B[ip+nip*id] = shape(orig_id);
      }
   }
}

template <typename real_t>
void CalcGradTensor(const FiniteElement &fe, const IntegrationRule &ir,
                    real_t *G, const Array<int> *dof_map = NULL)
{
   // - G must be (nip x dim x dof) with column major storage
   // - The inverse of dof_map is applied to reorder the local dofs.
   int dim = fe.GetDim();
   int nip = ir.GetNPoints();
   int dof = fe.GetDof();
   DenseMatrix dshape(dof, dim);

   for (int ip = 0; ip < nip; ip++)
   {
      fe.CalcDShape(ir.IntPoint(ip), dshape);
      for (int id = 0; id < dof; id++)
      {
         int orig_id = dof_map ? (*dof_map)[id] : id;
         for (int d = 0; d < dim; d++)
         {
            G[ip+nip*(d+dim*id)] = dshape(orig_id, d);
         }
      }
   }
}

template <typename real_t>
void CalcShapes(const FiniteElement &fe, const IntegrationRule &ir,
                real_t *B, real_t *G, const Array<int> *dof_map)
{
   if (B) { mfem::CalcShapeMatrix(fe, ir, B, dof_map); }
   if (G) { mfem::CalcGradTensor(fe, ir, G, dof_map); }
}

template <Geometry::Type G, int P>
class H1_FiniteElement;

template <int P>
class H1_FiniteElement<Geometry::SEGMENT, P>
{
public:
   static const Geometry::Type geom = Geometry::SEGMENT;
   static const int dim    = 1;
   static const int degree = P;
   static const int dofs   = P+1;

   static const bool tensor_prod = true;
   static const int  dofs_1d     = P+1;

   // Type for run-time parameter for the constructor
   typedef H1_FECollection::BasisType parameter_type;

protected:
   const FiniteElement *my_fe;
   const Array<int> *my_dof_map;
   parameter_type type; // run-time specified basis type
   void Init(const parameter_type type_)
   {
      type = type_;
      if (type == H1_FECollection::GaussLobatto)
      {
         H1_SegmentElement *fe = new H1_SegmentElement(P);
         my_fe = fe;
         my_dof_map = &fe->GetDofMap();
      }
      else if (type == H1_FECollection::Positive)
      {
         H1Pos_SegmentElement *fe = new H1Pos_SegmentElement(P);
         my_fe = fe;
         my_dof_map = &fe->GetDofMap();
      }
      else
      {
         MFEM_ABORT("invalid basis type!");
      }
   }

public:
   H1_FiniteElement(const parameter_type type_ = H1_FECollection::GaussLobatto)
   {
      Init(type_);
   }
   H1_FiniteElement(const FiniteElementCollection &fec)
   {
      const H1_FECollection *h1_fec =
         dynamic_cast<const H1_FECollection *>(&fec);
      MFEM_ASSERT(h1_fec, "invalid FiniteElementCollection");
      Init(h1_fec->GetBasisType());
   }
   ~H1_FiniteElement() { delete my_fe; }

   template <typename real_t>
   void CalcShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      mfem::CalcShapes(*my_fe, ir, B, G, my_dof_map);
   }
   template <typename real_t>
   void Calc1DShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      CalcShapes(ir, B, G);
   }
   const Array<int> *GetDofMap() const { return my_dof_map; }
};

template <int P>
class H1_FiniteElement<Geometry::TRIANGLE, P>
{
public:
   static const Geometry::Type geom = Geometry::TRIANGLE;
   static const int dim    = 2;
   static const int degree = P;
   static const int dofs   = ((P + 1)*(P + 2))/2;

   static const bool tensor_prod = false;

   // Type for run-time parameter for the constructor
   typedef H1_FECollection::BasisType parameter_type;

protected:
   const FiniteElement *my_fe;
   parameter_type type; // run-time specified basis type
   void Init(const parameter_type type_)
   {
      type = type_;
      if (type == H1_FECollection::GaussLobatto)
      {
         my_fe = new H1_TriangleElement(P);
      }
      else if (type == H1_FECollection::Positive)
      {
         my_fe = new H1Pos_TriangleElement(P);
      }
      else
      {
         MFEM_ABORT("invalid basis type!");
      }
   }

public:
   H1_FiniteElement(const parameter_type type_ = H1_FECollection::GaussLobatto)
   {
      Init(type_);
   }
   H1_FiniteElement(const FiniteElementCollection &fec)
   {
      const H1_FECollection *h1_fec =
         dynamic_cast<const H1_FECollection *>(&fec);
      MFEM_ASSERT(h1_fec, "invalid FiniteElementCollection");
      Init(h1_fec->GetBasisType());
   }
   ~H1_FiniteElement() { delete my_fe; }

   template <typename real_t>
   void CalcShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      mfem::CalcShapes(*my_fe, ir, B, G, NULL);
   }
   const Array<int> *GetDofMap() const { return NULL; }
};

template <int P>
class H1_FiniteElement<Geometry::SQUARE, P>
{
public:
   static const Geometry::Type geom = Geometry::SQUARE;
   static const int dim     = 2;
   static const int degree  = P;
   static const int dofs    = (P+1)*(P+1);

   static const bool tensor_prod = true;
   static const int dofs_1d = P+1;

   // Type for run-time parameter for the constructor
   typedef H1_FECollection::BasisType parameter_type;

protected:
   const FiniteElement *my_fe, *my_fe_1d;
   const Array<int> *my_dof_map;
   parameter_type type; // run-time specified basis type
   void Init(const parameter_type type_)
   {
      type = type_;
      if (type == H1_FECollection::GaussLobatto)
      {
         H1_QuadrilateralElement *fe = new H1_QuadrilateralElement(P);
         my_fe = fe;
         my_dof_map = &fe->GetDofMap();
         my_fe_1d = new L2_SegmentElement(P, 1);
      }
      else if (type == H1_FECollection::Positive)
      {
         H1Pos_QuadrilateralElement *fe = new H1Pos_QuadrilateralElement(P);
         my_fe = fe;
         my_dof_map = &fe->GetDofMap();
         my_fe_1d = new L2Pos_SegmentElement(P);
      }
      else
      {
         MFEM_ABORT("invalid basis type!");
      }
   }

public:
   H1_FiniteElement(const parameter_type type_ = H1_FECollection::GaussLobatto)
   {
      Init(type_);
   }
   H1_FiniteElement(const FiniteElementCollection &fec)
   {
      const H1_FECollection *h1_fec =
         dynamic_cast<const H1_FECollection *>(&fec);
      MFEM_ASSERT(h1_fec, "invalid FiniteElementCollection");
      Init(h1_fec->GetBasisType());
   }
   ~H1_FiniteElement() { delete my_fe; delete my_fe_1d; }

   template <typename real_t>
   void CalcShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      mfem::CalcShapes(*my_fe, ir, B, G, my_dof_map);
   }
   template <typename real_t>
   void Calc1DShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      mfem::CalcShapes(*my_fe_1d, ir, B, G, NULL);
   }
   const Array<int> *GetDofMap() const { return my_dof_map; }
};

template <int P>
class H1_FiniteElement<Geometry::TETRAHEDRON, P>
{
public:
   static const Geometry::Type geom = Geometry::TETRAHEDRON;
   static const int dim    = 3;
   static const int degree = P;
   static const int dofs   = ((P + 1)*(P + 2)*(P + 3))/6;

   static const bool tensor_prod = false;

   // Type for run-time parameter for the constructor
   typedef H1_FECollection::BasisType parameter_type;

protected:
   const FiniteElement *my_fe;
   parameter_type type; // run-time specified basis type
   void Init(const parameter_type type_)
   {
      type = type_;
      if (type == H1_FECollection::GaussLobatto)
      {
         my_fe = new H1_TetrahedronElement(P);
      }
      else if (type == H1_FECollection::Positive)
      {
         my_fe = new H1Pos_TetrahedronElement(P);
      }
      else
      {
         MFEM_ABORT("invalid basis type!");
      }
   }

public:
   H1_FiniteElement(const parameter_type type_ = H1_FECollection::GaussLobatto)
   {
      Init(type_);
   }
   H1_FiniteElement(const FiniteElementCollection &fec)
   {
      const H1_FECollection *h1_fec =
         dynamic_cast<const H1_FECollection *>(&fec);
      MFEM_ASSERT(h1_fec, "invalid FiniteElementCollection");
      Init(h1_fec->GetBasisType());
   }
   ~H1_FiniteElement() { delete my_fe; }

   template <typename real_t>
   void CalcShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      mfem::CalcShapes(*my_fe, ir, B, G, NULL);
   }
   const Array<int> *GetDofMap() const { return NULL; }
};

template <int P>
class H1_FiniteElement<Geometry::CUBE, P>
{
public:
   static const Geometry::Type geom = Geometry::CUBE;
   static const int dim     = 3;
   static const int degree  = P;
   static const int dofs    = (P+1)*(P+1)*(P+1);

   static const bool tensor_prod = true;
   static const int dofs_1d = P+1;

   // Type for run-time parameter for the constructor
   typedef H1_FECollection::BasisType parameter_type;

protected:
   const FiniteElement *my_fe, *my_fe_1d;
   const Array<int> *my_dof_map;
   parameter_type type; // run-time specified basis type

   void Init(const parameter_type type_)
   {
      type = type_;
      if (type == H1_FECollection::GaussLobatto)
      {
         H1_HexahedronElement *fe = new H1_HexahedronElement(P);
         my_fe = fe;
         my_dof_map = &fe->GetDofMap();
         my_fe_1d = new L2_SegmentElement(P, 1);
      }
      else if (type == H1_FECollection::Positive)
      {
         H1Pos_HexahedronElement *fe = new H1Pos_HexahedronElement(P);
         my_fe = fe;
         my_dof_map = &fe->GetDofMap();
         my_fe_1d = new L2Pos_SegmentElement(P);
      }
      else
      {
         MFEM_ABORT("invalid basis type!");
      }
   }

public:
   H1_FiniteElement(const parameter_type type_ = H1_FECollection::GaussLobatto)
   {
      Init(type_);
   }
   H1_FiniteElement(const FiniteElementCollection &fec)
   {
      const H1_FECollection *h1_fec =
         dynamic_cast<const H1_FECollection *>(&fec);
      MFEM_ASSERT(h1_fec, "invalid FiniteElementCollection");
      Init(h1_fec->GetBasisType());
   }
   ~H1_FiniteElement() { delete my_fe; delete my_fe_1d; }

   template <typename real_t>
   void CalcShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      mfem::CalcShapes(*my_fe, ir, B, G, my_dof_map);
   }
   template <typename real_t>
   void Calc1DShapes(const IntegrationRule &ir, real_t *B, real_t *G) const
   {
      mfem::CalcShapes(*my_fe_1d, ir, B, G, NULL);
   }
   const Array<int> *GetDofMap() const { return my_dof_map; }
};

} // namespace mfem

#endif // MFEM_TEMPLATE_FINITE_ELEMENTS_H1
