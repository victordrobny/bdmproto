//
//  spatial_tree.hpp
//  TreeComp
//
//  Created by Victor Drobny on 18.10.16.
//  Copyright © 2016 Victor Drobny. All rights reserved.
//

#ifndef spatial_tree_hpp
#define spatial_tree_hpp

#include <vector>
#include <utility>
#include "utils.hpp"
#include <stdio.h>

using std::vector;
using std::pair;

template <typename T>
class spatial_tree_node
{
protected:
    bound bnd;
    
private:
    virtual spatial_tree_node<T> ** get_children() = 0;
    virtual int get_children_size() = 0;
    virtual vector<pair<point, T> > * get_objects() = 0;
    static void get_neighbors_(
                               spatial_tree_node<T> * A,
                               spatial_tree_node<T> * B, 
                               double distance,
                               vector<pair<pair<point, T>, pair<point, T> > > * result);
    
public:
    
    spatial_tree_node() {}
    virtual ~spatial_tree_node() {}
    bound get_bound();
    virtual bool is_leaf() = 0;
    virtual void put(point p, T obj) = 0;

    
    virtual vector<pair<pair<point, T>, pair<point, T> > > * get_neighbors(double distance);
};

template <typename T>
bound spatial_tree_node<T>::get_bound()
{
    return bnd;
}

template <typename T>
vector<pair<pair<point, T>, pair<point, T> > > * spatial_tree_node<T>::get_neighbors(double distance)
{
    vector<pair<pair<point, T>, pair<point, T> > > * result = new vector<pair<pair<point, T>, pair<point, T> > >();
    get_neighbors_(this, this, distance * distance, result);
    return result;
}


template <typename T>
pair<pair<point, T>, pair<point, T> > make_neighbor_pair(point p1, T o1, point p2, T o2)
{
    return make_pair(make_pair(p1, o1), make_pair(p2, o2));
}


template <typename T>
pair<pair<point, T>, pair<point, T> > make_neighbor_pair(pair<point, T> p1, pair<point ,T> p2)
{
    return make_pair(make_pair(p1.first, p1.second), make_pair(p2.first, p2.second));
}


template <typename T>
void spatial_tree_node<T>::get_neighbors_(
                                            spatial_tree_node<T> * A,
                                            spatial_tree_node<T> * B,
                                            double distance,
                                            vector<pair<pair<point, T>, pair<point, T> > > * result)
{
    if (A->is_leaf() && B->is_leaf())
    {
        auto a_objs = A->get_objects();
        auto b_objs = B->get_objects();
        
        bool is_same = A == B;
        
        for (int i = 0; i < a_objs->size(); i++)
            for (int j = is_same ? (i + 1) : 0; j < b_objs->size(); j++)
                if ((a_objs->at(i).first).sqdist(b_objs->at(j).first) <= distance)
                    result->push_back(make_neighbor_pair(a_objs->at(i), b_objs->at(j)));
    }
    else
    {
        spatial_tree_node<T> ** a_nodes, ** b_nodes;
        bool a_allocated = false, b_allocated = false;
        int a_size = 0, b_size = 0;
        if (A->is_leaf())
        {
            a_nodes = new spatial_tree_node<T> *[1];
            a_nodes[0] = A;
            a_size = 1;
            a_allocated = true;
        }
        else
        {
            a_nodes = A->get_children();
            a_size = A->get_children_size();
            a_allocated = false;
        }
        
        if (B->is_leaf())
        {
            b_nodes = new spatial_tree_node<T> *[1];
            b_nodes[0] = B;
            b_size = 1;
            b_allocated = true;
        }
        else
        {
            b_nodes = B->get_children();
            b_size = B->get_children_size();
            b_allocated = false;
        }
        
        bool is_same = A == B;
        for (int i = 0; i < a_size; i++)
        {
            for (int j = is_same ? (i) : 0; j < b_size; j++)
            {
                if (a_nodes[i]->get_bound().distance(b_nodes[j]->get_bound()) <= distance)
                {
                    get_neighbors_(a_nodes[i], b_nodes[j], distance, result);
                }
            }
        }
    }
}
