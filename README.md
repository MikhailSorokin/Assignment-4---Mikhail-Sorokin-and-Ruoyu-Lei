Authors - Mikhail Sorokin and Ruoyu Lei

Programming Assignment 4: Materials, Texture, and Transformations
----------
#### The following methods are implemented:

1. [Camera Parameter Animations](#camera)
2. [Materials](#materials)
3. [Groups](#groups)
4. [BONUS](#bonus)

# Camera

There are four parts of this:

- FOV Animation. Incrementing the field of view makes all objects appear smaller, while decreasing it makes the objects appear bigger.

- Near Plane. Increasing the near plane makes the objects disappear from the front, while decreasing it makes objects appear.

- Far Plane. Increasing the near plane makes the objects disappear from the back, while decreasing it makes objects appear from the back.

- Animate Camera. 

# Materials

TO BE DONE

# Groups

TO BE DONE


# BONUS

We tried to implement bilateral filter and did researches in 3 papers on this topic. Although not succeed because there is something glitchy about our implementation that makes the program keep crashing, we'd like to wirte down some insights on this algorithm. 

The bilateral introduced in the [paper](http://people.csail.mit.edu/thouis/JDD03.pdf) linked in the spec and [another paper](http://mesh.brown.edu/DGP/pdfs/Fleishman-sg03.pdf) by Fleishman [2003] suggest that the 3D bilateral filter evolved from the 2D bilateral filter by Black et al. [1998]. It is an algorithm used to smooth the surface of a mesh using Gaussian statistics.

The formula is given as:

For each vertex p, the smoothed vertex p` is computed by

![foo](img_before/formula3.png)

where k(p) is denoted:

![foo](img_before/formula4.png)

Here is a detailed breakdown of the forumla and how to implement it:

The summation notation suggests that in order to get p` for a vertex p, it has to iterate through all faces. Therefore the time complexity of this algorithm would be O(n * m), where n is the number of vertices and m is the number of faces.

The first element Πq(p) we need to figure out is the predictor. It is the projection of p to the plane tangent to q. As shown below:

![foo](img_before/tangent.png)

It can be achieved by finding the intersection of the perpendicular through the point with the tangent plane. In other words, it’s the nearest point on the tangent plane to the given point.

Alternatively, [Fleishman [2003]](http://mesh.brown.edu/DGP/pdfs/Fleishman-sg03.pdf) provided a pesudocode fragment in the paper to demostrate how this Πq(p) can be calculated. Notice this method does not iterate all surface, but only the onces that are neighbors to the point.

![foo](img_before/pesudocode.png)

aq is the area of the surface q.

Next we move on to f and g, spatial weight and influence weight. They control amount of smoothness and "how far" we'd like to spread the effect.

To find f, we first find the distance ||p − cq || between p and the centroid cq of q. Then we put the distance into gaussian function to get f.  The σf in guassian is very important. We will explain it after.

To find, we use a similar approch. First find the distance ||Πq (p) − p|| between Πq(p) and the position of p, and then use a gaussian.

Then we move on to k(p). This is very similar to the summation above. The prupose of doing 1/k(p) is to devide the weighted and influenced sum with their weights.

Last but not least, we need to mollify this function to make it perform well. As explained earlier, σf is very important because it determines the quality of smoothing. Previously, [Durand and Dorsey [2002]](https://people.csail.mit.edu/fredo/PUBLI/Siggraph2002/DurandBilateral.pdf) uses 1/5 of the mean edge over all edges as σf. It works decently but we can mollify it the make it perform better. This is done using σf = 1/2 σf as introduced in this paper. There are some sample images at the end of the paper to illustrate the effect influenced by change of σf.