#ifndef MESH_H_
#define MESH_H_

#include <player.pb.h>
#include <boost/shared_ptr.hpp>

class IFactory;
class IMeshBase;

class Mesh
{
public: Mesh( IFactory&, const ::duke::protocol::Mesh& );
	virtual ~Mesh();

	void render( IFactory& ) const;

private:
	::boost::shared_ptr<IMeshBase> m_pMeshBase;
};

#endif /* MESH_H_ */
