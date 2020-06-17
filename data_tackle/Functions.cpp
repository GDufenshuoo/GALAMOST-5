/*
GALAMOST - GPU-Accelerated Large-Scale Molecular Simulation Toolkit
Version 5
COPYRIGHT
	GALAMOST Copyright (c) (2020) G.U.G.D.
LICENSE
	This program is a free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License. 
	This program is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANT ABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
	See the General Public License v3 for more details.
	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
DISCLAIMER
	The authors of GALAMOST do not guarantee that this program and its 
	derivatives are free from error. In no event shall the copyright 
	holder or contributors be liable for any indirect, incidental, 
	special, exemplary, or consequential loss or damage that results 
	from its use. We also have no responsibility for providing the 
	service of functional extension of this program to general users.
USER OBLIGATION 
	If any results obtained with GALAMOST are published in the scientific 
	literature, the users have an obligation to distribute this program 
	and acknowledge our efforts by citing the paper "Y.-L. Zhu et al.,
	J. Comput. Chem. 2013,34, 2197-2211" in their article.
CORRESPONDENCE
	Dr. You-Liang Zhu, 
	Email: ylzhu@galamost.com
*/

#include "Functions.h"
#include "cuComplex.h"


	
double rsq_cal(std::string direction, double dx, double dy, double dz)
	{
	double rsq = dx*dx + dy*dy + dz*dz;		
	if(direction=="X")
		rsq = dx*dx;
	else if(direction=="Y")
		rsq = dy*dy;
	else if(direction=="Z")
		rsq = dz*dz;	
	else if(direction=="XY")
		rsq = dx*dx + dy*dy;
	else if(direction=="YZ")
		rsq = dy*dy + dz*dz;	
	else if(direction=="XZ")
		rsq = dx*dx + dz*dz;
	return rsq;
	};
	
//--- case 1
void Rg2::compute()
	{
	std::vector<vec> pos0 = m_mol->getPos0();
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	std::vector< double > mass = m_build->getMass();
	if(mass.size()==0)
		{
        cout<< "***Warning! no input mass, set particle mass to be 1.0!" << endl << endl;
		mass.resize(pos0.size());
		for(unsigned int i=0; i< mass.size(); i++)
			mass[i]=1.0;
		}
	
	unsigned int n_kind_mol = m_mol->getKindNumofMol();
	std::vector<unsigned int > n_mol_per_kind = m_mol->getNmolperKind();
	std::vector< double > grPerMol;
	std::vector< double > massPerMol;	
	std::vector< vec > center_per_mol;
	std::vector< double > grPerMolKind;	
	grPerMol.resize(mol_type_id.size());
	massPerMol.resize(mol_type_id.size());
	center_per_mol.resize(mol_type_id.size());
	grPerMolKind.resize(n_kind_mol);
	
	if(m_Nf==0)
		m_av_rg2.resize(n_kind_mol);
	for (unsigned int i = 0; i< pos0.size(); i++)
		{
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			double massi = mass[i];
			center_per_mol[molid].x += massi*pos0[i].x;
			center_per_mol[molid].y += massi*pos0[i].y;		
			center_per_mol[molid].z += massi*pos0[i].z;
			massPerMol[molid] += massi;
			}
		}
	for (unsigned int i = 0; i< center_per_mol.size(); i++)
		{
		center_per_mol[i].x /= (double)massPerMol[i];
		center_per_mol[i].y /= (double)massPerMol[i];		
		center_per_mol[i].z /= (double)massPerMol[i];	
		}		
	for (unsigned int i = 0; i< pos0.size(); i++)
		{
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{		
			vec center = center_per_mol[molid];
			double massi = mass[i];
			double gr2 = (pos0[i].x- center.x)*(pos0[i].x- center.x) + (pos0[i].y- center.y)*(pos0[i].y- center.y) + (pos0[i].z- center.z)*(pos0[i].z- center.z);
			grPerMol[molid] +=gr2*massi;
			}
		}		
	for (unsigned int i = 0; i< grPerMol.size(); i++)
		{
		grPerMol[i] /= massPerMol[i];
		unsigned int moltypeid = mol_type_id[i];
		grPerMolKind[moltypeid] += grPerMol[i];
		}	
	for (unsigned int i = 0; i< grPerMolKind.size(); i++)
		{
		grPerMolKind[i] /= (double)n_mol_per_kind[i];
		m_av_rg2[i] += grPerMolKind[i];
		}
			
	std::string fname = m_build->getFilename();	
	m_file <<fname;
	for(unsigned int j =0; j< grPerMolKind.size(); j++)
		m_file <<"  Mol"<<j<<" "<<grPerMolKind[j]; 
	m_file <<"\n";
	m_Nf +=1;
	}
//--- case 2	
void Ed2::compute()
	{
	std::vector<vec> pos0 = m_mol->getPos0();
	std::vector<uint_2> molstart_end = m_mol->getMolStartEnd();
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	unsigned int n_kind_mol = m_mol->getKindNumofMol();
	std::vector<unsigned int > n_mol_per_kind = m_mol->getNmolperKind();
	std::vector< double > edPerMol;
	std::vector< double > edPerMolKind;	
	edPerMol.resize(mol_type_id.size());
	edPerMolKind.resize(n_kind_mol);
	if(m_Nf==0)
		m_av_ed2.resize(n_kind_mol);	
	for (unsigned int i = 0; i< molstart_end.size(); i++)
		{
		unsigned int pstart = molstart_end[i].x;
		unsigned int pend = molstart_end[i].y;	
		double dx = pos0[pend].x - pos0[pstart].x;
		double dy = pos0[pend].y - pos0[pstart].y;		
		double dz = pos0[pend].z - pos0[pstart].z;
		double edsq = dx*dx + dy*dy + dz*dz;
		edPerMol[i] = edsq;
		}
	for (unsigned int i = 0; i< edPerMol.size(); i++)
		{
		unsigned int moltypeid = mol_type_id[i];
		edPerMolKind[moltypeid] += edPerMol[i];
		}	
	for (unsigned int i = 0; i< edPerMolKind.size(); i++)
		{
		edPerMolKind[i] /= (double)n_mol_per_kind[i];
		m_av_ed2[i] += edPerMolKind[i];		
		}
	std::string fname = m_build->getFilename();		
	m_file <<fname;
	for(unsigned int j =0; j< edPerMolKind.size(); j++)
		m_file <<"  Mol"<<j<<" "<<edPerMolKind[j]; 
	m_file <<"\n";
	m_Nf +=1;	
	}
//--- case 3
 extern "C"{ 
cudaError_t gpu_compute_rdf(float4 *dpos,
                            unsigned int N,
							unsigned int N_total,
                            float Lx,
							float Ly,
							float Lz,
							float Lxinv,
							float Lyinv,
							float Lzinv,
							float delr,
							unsigned int *gg,
							unsigned int *d_scratch,
							unsigned int maxbin,
							unsigned int* d_group,
							unsigned int* d_n_exclusion,
							unsigned int* d_exclusion_list,
							unsigned int* d_mol_id_per_particle,
							bool exclusion_mol,
							bool exclusion_list,							
							bool bytype,
							unsigned int block_size);
} 


void RDF::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "rdf");
	else
		outs = fname+".rdf";
	ofstream fp(outs.c_str());
	
	unsigned int N = m_build->getNParticles();
//	unsigned int Ntypes = m_build->getNParticleTypes();

	BoxSize box = m_build->getBox();
	std::vector<vec> pos = m_build->getPos();
	std::vector<unsigned int> type = m_build->getType();
	unsigned int ndimension = m_build->getNDimensions();
		
	float Lx = float (box.lx);
	float Ly = float (box.ly);		 
	float Lz = float (box.lz);
	
	float Lxinv = 0.0;
	float Lyinv = 0.0;
	float Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;	


	int nblocks = (int)ceil((float)N / (float)m_block_size);
	double *r_DPD = (double*) calloc(m_maxbin,sizeof(double));	
	double *g = (double*) calloc(m_maxbin,sizeof(double));
	
 	float4 *hpos;
  	float4 *dpos;
	unsigned int* h_group;
	unsigned int* d_group;	
	unsigned int* scratch;
	unsigned int* d_scratch;
	unsigned int* d_gg;
	int nbytes = sizeof(float)*N*4;
	cudaHostAlloc(&hpos, nbytes, cudaHostAllocPortable);
	cudaHostAlloc(&h_group, sizeof(unsigned int)*N, cudaHostAllocPortable);		
	
	cudaHostAlloc(&scratch, sizeof(unsigned int)*nblocks*m_maxbin, cudaHostAllocPortable);		
    cudaMalloc(&dpos, nbytes);
	cudaMalloc(&d_group, sizeof(unsigned int)*N);	
	cudaMalloc(&d_scratch, sizeof(unsigned int)*nblocks*m_maxbin);
	cudaMemset(d_scratch, 0, sizeof(unsigned int)*nblocks*m_maxbin);		
	cudaMalloc(&d_gg, sizeof(unsigned int)*nblocks*m_maxbin*m_block_size);
	cudaMemset(d_gg, 0, sizeof(unsigned int)*nblocks*m_maxbin*m_block_size);
		
	for(unsigned int i =0; i< N; i++)
		{
		hpos[i].x = float(pos[i].x);
		hpos[i].y = float(pos[i].y);
		hpos[i].z = float(pos[i].z);
		hpos[i].w = float(type[i]);
		h_group[i] = i;
		}	
	cudaMemcpy(dpos, hpos, nbytes, cudaMemcpyHostToDevice);
	cudaMemcpy(d_group, h_group, sizeof(unsigned int)*N, cudaMemcpyHostToDevice);
	float pi = 4.0*atan(1.0);
        
    float rmax = 0.5*Lx;
	if (m_rmax>0.0)
		rmax = m_rmax;
	float rho  = (float)(N)*Lxinv*Lyinv*Lzinv;
	if(ndimension==2)
		rho = (float)(N)*Lxinv*Lyinv;
	float delr = rmax/(float)m_maxbin;
	bool bytype = false;

	if (m_exclusion_mol)
		{
		d_mol_id_per_particle = m_mol->getMolIdPerParticleGPU();
		}
	if(m_exclusion_bond)
		{
		m_mol->bondExclude();
		m_exclusion_list=true;
		}
	if(m_exclusion_angle)
		{
		m_mol->angleExclude();
		m_exclusion_list=true;
		}		
	if (m_exclusion_list)
		{
		d_n_exclusion = m_mol->getNExclusionGPU();
		d_exclusion_list = m_mol->getExclusionListGPU();
		}					 
	gpu_compute_rdf(dpos,N,N,Lx,Ly,Lz,Lxinv,Lyinv,Lzinv,delr,d_gg,d_scratch,m_maxbin,d_group,
					d_n_exclusion,d_exclusion_list,d_mol_id_per_particle,m_exclusion_mol,
					m_exclusion_list,bytype,m_block_size);

	cudaMemcpy(scratch, d_scratch, sizeof(unsigned int)*nblocks*m_maxbin, cudaMemcpyDeviceToHost);

	for(unsigned int i =0; i<(unsigned int)nblocks;i++)
		{
		for(unsigned int j =0; j< m_maxbin;j++)
			g[j] += double(scratch[i*m_maxbin+j]);
		}

    double con = 4.0 * pi*rho/3.0;
	if(ndimension==2)
		con = pi*rho;
	for (unsigned int bin = 0; bin < m_maxbin; bin++ )
		{ 
	  	double rlower = (double)(bin)*delr;
	    double rupper = rlower + delr;
	    r_DPD[bin] = rlower + 0.5*delr;
	    double nid = con*(rupper*rupper*rupper-rlower*rlower*rlower);
		if(ndimension==2)
			nid = con*(rupper*rupper-rlower*rlower);
	    g[bin] /= (double)(N)*nid;
        fp<<r_DPD[bin]<<"  "<<g[bin]<<"\n";
		m_rdf[bin] += g[bin];
		}
	fp.close();
	if(m_Nf==0)
		{
		for(unsigned int i=0; i<m_maxbin; i++ )
			m_r[i]=r_DPD[i];
		}	
	cudaFreeHost(hpos);
 	cudaFree(dpos);

	cudaFreeHost(scratch);
 	cudaFree(d_scratch);	
 	cudaFree(d_gg);	
	m_Nf += 1;	
	} 
//--- case 4
void Bond_distr::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "bond_distr");
	else
		outs = fname+".bond_distr";
	ofstream fp(outs.c_str());
	
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);	
	
	double Lxinv = 0.0;
	double Lyinv = 0.0;
	double Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;	

	if (m_Nf==0)
		{
		double maxL =  (((Lx) > (Ly)) ? (Lx) : (Ly));
		maxL =  (((maxL) > (Lz)) ? (maxL) : (Lz));
		m_rcut = 0.5*maxL;
		m_delt = m_rcut/double(m_Npot-1);		
		m_Nb = m_build->getNBondTypes();
		m_distr.resize(m_Npot*m_Nb);
		m_bond_lenth.resize(m_Nb);
		m_bondMap = m_build->getBondTypeMap();		
		}

	std::vector<vec> pos = m_build->getPos();
	std::vector<Bond> bonds = m_build->getBond();
	std::vector<unsigned int > distribute;
	std::vector<unsigned int > numb;
	std::vector<double> bond_lenth;	
	distribute.resize(m_Npot*m_Nb);
	numb.resize(m_Nb);		
	bond_lenth.resize(m_Nb);

	for(unsigned int i=0; i<bonds.size();i++)
		{
		double dx = pos[bonds[i].a].x - pos[bonds[i].b].x;
		double dy = pos[bonds[i].a].y - pos[bonds[i].b].y;		
		double dz = pos[bonds[i].a].z - pos[bonds[i].b].z;
		dx-= Lx * rint(dx*Lxinv);
		dy-= Ly * rint(dy*Lyinv);
		dz-= Lz * rint(dz*Lzinv);
		double rsq = dx*dx + dy*dy + dz*dz;
		double r = sqrt(rsq);
		if(r>=double(m_Npot)*m_delt)
			{
			cout<<"Error, rcut is too small!"<<endl;
			cout<<r<<" "<<m_bondMap[bonds[i].id]<<" "<<bonds[i].a<<" "<<bonds[i].b<<endl;	
			}
		unsigned int p = bonds[i].id*m_Npot + int(r/m_delt);
		bond_lenth[bonds[i].id] += r;
		distribute[p] += 1;
		}	
	for(unsigned int i=0; i<m_Nb;i++)
		{
		for(unsigned int j=0;j<m_Npot;j++)
			numb[i] += distribute[i*m_Npot+j];
		}		
		
	for(unsigned int i=0; i<m_Nb;i++)
		{
		fp<<m_bondMap[i]<<endl;
		m_bond_lenth[i] += bond_lenth[i]/double(numb[i]);
		for(unsigned int j=0;j<m_Npot;j++)
			{
			if(distribute[i*m_Npot+j]>0)
				{
				double value = double(distribute[i*m_Npot+j])/double(numb[i]);
				fp<<double(j)*m_delt<<"  "<<value/m_delt<<"\n";
				m_distr[i*m_Npot+j] += value;
				}
			}
		}		
	fp.close();
	m_Nf += 1;
	} 
//--- case 5	
void Angle_distr::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "angle_distr");
	else
		outs = fname+".angle_distr";
	ofstream fp(outs.c_str());
	
	if (m_Nf==0)
		{
		double rcut = M_PI;
		m_delt = rcut/double(m_Npot-1);		
		m_Nb = m_build->getNAngleTypes();
		m_distr.resize(m_Npot*m_Nb);
		m_angle_radian.resize(m_Nb);		
		m_angleMap = m_build->getAngleTypeMap();	
		}
		
	BoxSize box = m_build->getBox();
	std::vector<vec> pos = m_build->getPos();
	std::vector<Angle> angles = m_build->getAngle();
	std::vector<unsigned int > distribute;
	std::vector<unsigned int > numb;
	std::vector<double> angle_radian;		
	distribute.resize(m_Npot*m_Nb);
	numb.resize(m_Nb);		
	angle_radian.resize(m_Nb);
	
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);	

	double Lxinv = 0.0;
	double Lyinv = 0.0;
	double Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;		

	for(unsigned int i=0; i<angles.size();i++)
		{
		double dxab = pos[angles[i].a].x - pos[angles[i].b].x;
		double dyab = pos[angles[i].a].y - pos[angles[i].b].y;		
		double dzab = pos[angles[i].a].z - pos[angles[i].b].z;

		double dxcb = pos[angles[i].c].x - pos[angles[i].b].x;
		double dycb = pos[angles[i].c].y - pos[angles[i].b].y;		
		double dzcb = pos[angles[i].c].z - pos[angles[i].b].z;		

        dxab -= Lx * rint(dxab*Lxinv);
        dyab -= Ly * rint(dyab*Lyinv);
        dzab -= Lz * rint(dzab*Lzinv);
        
        dxcb -= Lx * rint(dxcb*Lxinv);		
        dycb -= Ly * rint(dycb*Lyinv);		
        dzcb -= Lz * rint(dzcb*Lzinv);

        double rsqab = dxab*dxab+dyab*dyab+dzab*dzab;
        double rab = sqrt(rsqab);
        double rsqcb = dxcb*dxcb+dycb*dycb+dzcb*dzcb;
        double rcb = sqrt(rsqcb);		
		
        double c_abbc = dxab*dxcb+dyab*dycb+dzab*dzcb;
        c_abbc /= rab*rcb;
        if (c_abbc > 1.0) c_abbc = 1.0;
        if (c_abbc < -1.0) c_abbc = -1.0;		
		double th = acos(c_abbc);
		int pth = int(th/m_delt);
		if(pth>=int(m_Npot)||pth<0)
			{
			cout<<rab<<" "<<rcb<<" "<<angles[i].a<<" "<<angles[i].b<<" "<<angles[i].c<<endl;
			cout<<"Error!! angle theta = "<<th<<" is too large or less than zero!"<<endl;
			}
		unsigned int p = angles[i].id*m_Npot + pth;
		angle_radian[angles[i].id] += th;		
		distribute[p] += 1;
		}	
	for(unsigned int i=0; i<m_Nb;i++)
		{
		for(unsigned int j=0;j<m_Npot;j++)
			numb[i] += distribute[i*m_Npot+j];
		}		
		
	for(unsigned int i=0; i<m_Nb;i++)
		{
		fp<<m_angleMap[i]<<endl;
		m_angle_radian[i] += angle_radian[i]/double(numb[i]);		
		for(unsigned int j=0;j<m_Npot;j++)
			{
			if(distribute[i*m_Npot+j]>0)
				{
				double value = double(distribute[i*m_Npot+j])/double(numb[i]);
				fp<<double(j)*m_delt<<"  "<<value/m_delt<<"\n";
				m_distr[i*m_Npot+j] += value;
				}
			}
		}		
	fp.close();
	m_Nf += 1;
	} 
//--- case 6
void Dihedral_distr::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "dihedral_distr");
	else
		outs = fname+".dihedral_distr";
	ofstream fp(outs.c_str());	

	if (m_Nf==0)
		{
		double rcut = 2*M_PI;
		m_delt = rcut/double(m_Npot-1);		
		m_Nb = m_build->getNDihedralTypes();
		m_distr.resize(m_Npot*m_Nb);
		m_dihedral_radian.resize(m_Nb);		
		m_dihedralMap = m_build->getDihedralTypeMap();	
		}

	BoxSize box = m_build->getBox();
	std::vector<vec> pos = m_build->getPos();
	std::vector<Dihedral> dihedrals = m_build->getDihedral();
	std::vector<unsigned int > distribute;
	std::vector<unsigned int > numb;
	std::vector<double> dihedral_radian;		
	distribute.resize(m_Npot*m_Nb);
	numb.resize(m_Nb);
	dihedral_radian.resize(m_Nb);	
	
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);	
	
	double Lxinv = 0.0;
	double Lyinv = 0.0;
	double Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;	

	for(unsigned int i=0; i<dihedrals.size();i++)
		{
		double dxab = pos[dihedrals[i].a].x - pos[dihedrals[i].b].x;
		double dyab = pos[dihedrals[i].a].y - pos[dihedrals[i].b].y;		
		double dzab = pos[dihedrals[i].a].z - pos[dihedrals[i].b].z;

		double dxcb = pos[dihedrals[i].c].x - pos[dihedrals[i].b].x;
		double dycb = pos[dihedrals[i].c].y - pos[dihedrals[i].b].y;		
		double dzcb = pos[dihedrals[i].c].z - pos[dihedrals[i].b].z;	

		double dxdc = pos[dihedrals[i].d].x - pos[dihedrals[i].c].x;
		double dydc = pos[dihedrals[i].d].y - pos[dihedrals[i].c].y;		
		double dzdc = pos[dihedrals[i].d].z - pos[dihedrals[i].c].z;			

        dxab -= Lx * rint(dxab*Lxinv);
        dyab -= Ly * rint(dyab*Lyinv);
        dzab -= Lz * rint(dzab*Lzinv);
        
        dxcb -= Lx * rint(dxcb*Lxinv);		
        dycb -= Ly * rint(dycb*Lyinv);		
        dzcb -= Lz * rint(dzcb*Lzinv);

        dxdc -= Lx * rint(dxdc*Lxinv);		
        dydc -= Ly * rint(dydc*Lyinv);		
        dzdc -= Lz * rint(dzdc*Lzinv);
		
		
        double dxcbm = -dxcb;
        double dycbm = -dycb;
        double dzcbm = -dzcb;

        double aax = dyab*dzcbm - dzab*dycbm;
        double aay = dzab*dxcbm - dxab*dzcbm;
        double aaz = dxab*dycbm - dyab*dxcbm;
        
        double bbx = dydc*dzcbm - dzdc*dycbm;
        double bby = dzdc*dxcbm - dxdc*dzcbm;
        double bbz = dxdc*dycbm - dydc*dxcbm;
        
        double raasq = aax*aax + aay*aay + aaz*aaz;
        double rbbsq = bbx*bbx + bby*bby + bbz*bbz;
        double rgsq = dxcbm*dxcbm + dycbm*dycbm + dzcbm*dzcbm;
        double rg = sqrt(rgsq);
        
        double  raa2inv, rbb2inv;
        raa2inv = rbb2inv = 0.0f;
        if (raasq > 0.0f) raa2inv = 1.0f/raasq;
        if (rbbsq > 0.0f) rbb2inv = 1.0f/rbbsq;
        double rabinv = sqrt(raa2inv*rbb2inv);
        
        double c_abcd = (aax*bbx + aay*bby + aaz*bbz)*rabinv;
        double s_abcd = rg*rabinv*(aax*dxdc + aay*dydc + aaz*dzdc);
        
        if (c_abcd > 1.0f) c_abcd = 1.0f;
        if (c_abcd < -1.0f) c_abcd = -1.0f;

		double th = atan2f(s_abcd,c_abcd);		
		if (th<0)
			th += 2.0*M_PI;
		int pth = int(th/m_delt);
		if(pth>=int(m_Npot)||pth<0)
			cout<<"Error!! dihedral theta = "<<th<<" is too large or less than zero!"<<endl;
		unsigned int p = dihedrals[i].id*m_Npot + pth;
		dihedral_radian[dihedrals[i].id] += th;			
		distribute[p] += 1;
		}	

	for(unsigned int i=0; i<m_Nb;i++)
		{
		for(unsigned int j=0;j<m_Npot;j++)
			numb[i] += distribute[i*m_Npot+j];
		}		

	for(unsigned int i=0; i<m_Nb;i++)
		{
		fp<<m_dihedralMap[i]<<endl;
		m_dihedral_radian[i] += dihedral_radian[i]/double(numb[i]);
		for(unsigned int j=0;j<m_Npot;j++)
			{
			if(distribute[i*m_Npot+j]>0)
				{
				double value = double(distribute[i*m_Npot+j])/double(numb[i]);
				fp<<double(j)*m_delt<<"  "<< value/m_delt<<"\n";
				m_distr[i*m_Npot+j] += value;
				}
			}
		}		
	fp.close();
	m_Nf += 1;	
	} 
//--- case 7
unsigned int cellid(int i, int j, int k, vec_uint& m_dim)
	{
	i = (i + (int)m_dim.x)%(int)m_dim.x;
	j = (j + (int)m_dim.y)%(int)m_dim.y;
	k = (k + (int)m_dim.z)%(int)m_dim.z;	
	return (unsigned int) (i + j*m_dim.x + k*m_dim.x*m_dim.y);
	}
	
void StressTensor::setParam()
	{
	unsigned int Ntypes = m_build->getNParticleTypes();	
	unsigned int NBtypes = m_build->getNBondTypes();
	m_pparams.resize(Ntypes*Ntypes);
	m_bparams.resize(NBtypes);
	std::vector< std::string > typemap =  m_build->getTypeMap();
	std::vector< std::string > Btypemap =  m_build->getBondTypeMap();
	for(unsigned int typi=0; typi< Ntypes; typi++)
		{
		for(unsigned int typj=typi; typj< Ntypes; typj++)
			{
			cout<<"7.  epsilon  sigma  alpha  rcut("<<typemap[typi]<<"  "<<typemap[typj]<<")"<<endl;   
			double epsilon, sigma, alpha, rcut;
			cin >> epsilon>>sigma>>alpha>>rcut;
			if (rcut > m_rcut)
				m_rcut = rcut;
			double lj1 = 4.0 * epsilon * pow(sigma, int(12));
			double lj2 = alpha * 4.0 * epsilon * pow(sigma, int(6));
			m_pparams[typi*Ntypes + typj] = vec(lj1, lj2, rcut);
			m_pparams[typj*Ntypes + typi] = vec(lj1, lj2, rcut);
			}
		}
	for(unsigned int typi=0; typi< NBtypes; typi++)
		{						
		cout<<"7.  K0  R0 ("<<Btypemap[typi]<<")"<<endl;   
		double K0, R0;
		cin >> K0>>R0;
		m_bparams[typi] = vec(K0, R0, 0);
		}
	m_file <<"File Name                "<<" sigmaXX "<<" sigmaYY "<<" sigmaZZ "<<" sigmaXY "<<" sigmaYZ "<<" sigmaZX"<<" sigmavM"<<endl;
	cout<<"7.  Computing ... "<<endl;		
	}
	
void StressTensor::compute()
	{
	std::vector<double> diameter = m_build->getDiameter();
	if (m_diameter&&diameter.size()==0)
		{
		m_diameter = false;
		}
	if(m_Nf==0)
		{
		setParam();
		if (m_diameter)
			{
			for (unsigned int i=0; i< diameter.size(); i++)
				{
				if (diameter[i]>m_delt)
					m_delt = diameter[i];
				}
			m_delt -= 1.0;
			}
		}

	m_mol->computeList(m_rcut+m_delt);
	if(m_bondex)
		m_mol->bondExclude();
	std::vector<unsigned int> body = m_build->getBody();		
	if(m_bodyex&&body.size()==0)
		{
		m_bodyex=false;
		}	
	std::vector<vec> pos = m_mol->getPos();
	std::vector<vec_int> map = m_mol->getMap();	
	std::vector<unsigned int> m_head = m_mol->getHead();
	std::vector<unsigned int> m_list = m_mol->getList();	
	vec m_width = m_mol->getWith();
	vec_uint m_dim = m_mol->getDim();	
	std::vector<unsigned int> type = m_build->getType();
	std::vector<vec> vel = m_build->getVel();
	std::vector<double> mass = m_build->getMass();		
	unsigned int Ntypes = m_build->getNParticleTypes();
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	
	double Lxinv = 0.0;
	double Lyinv = 0.0;
	double Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;	
	
	double vol = Lx*Ly*Lz;
	if(vol==0.0)
		vol=Lx*Ly;
	std::vector< double > press;
	double virialXX = 0.0;
	double virialYY = 0.0;
	double virialZZ = 0.0;
	double virialXY = 0.0;
	double virialYZ = 0.0;
	double virialZX = 0.0;
	double vvXX = 0.0;
	double vvYY = 0.0;
	double vvZZ = 0.0;
	double vvXY = 0.0;
	double vvYZ = 0.0;
	double vvZX = 0.0;	
//-----------------nb force
	for (unsigned int idx = 0; idx< pos.size(); idx++)
		{
		unsigned int my_body = NO_BODY;
		if(m_bodyex)
			my_body = body[idx];
	    double px = pos[idx].x;
		double py = pos[idx].y;
		double pz = pos[idx].z;

		int ix = int((px+0.5*Lx)/m_width.x);
		int iy = int((py+0.5*Ly)/m_width.y);
		int iz = int((pz+0.5*Lz)/m_width.z);			
//		unsigned int cid = cellid(ix, iy, iz, m_dim);
//----------compute with particles in same cell
		double di = 0.0;
		if (m_diameter)
			di = diameter[idx];
		unsigned int id = m_list[idx];
		while(id!=NO_INDEX)
			{
			double dx = px - pos[id].x;
			double dy = py - pos[id].y;
			double dz = pz - pos[id].z;
			double shiftx = 0.0;
			double shifty = 0.0;
			double shiftz = 0.0;
							
			if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
				{
				shiftx = rint(dx*Lxinv);
				shifty = rint(dy*Lyinv);
				shiftz = rint(dz*Lzinv);
								
				dx -=  Lx *shiftx;
				dy -=  Ly *shifty;				
				dz -=  Lz *shiftz;
				}
			double rsq = dx*dx + dy*dy + dz*dz;				
			double r = sqrt(rsq);
			double deltij=0.0;
			if (m_diameter)
				{
				double dj = diameter[id];
				deltij = (di + dj)/2.0 - 1.0;
				}
			double rs = r - deltij;
			int typ_pair = type[idx] * Ntypes + type[id];

			double lj1 = m_pparams[typ_pair].x;
			double lj2 = m_pparams[typ_pair].y;
			double rcut = m_pparams[typ_pair].z;
			bool bexcluded =false; 
			if (m_bodyex&& my_body != NO_BODY)
				{
				unsigned int neigh_body = body[id];			
                bexcluded = (my_body == neigh_body);
				}
			if(r<rcut+deltij&&!m_mol->ifexclude(idx,id)&&!bexcluded)	
				{							
				double rinv = 1.0/r;
				double rsinv = 1.0/rs;
				double r2inv = rsinv * rsinv;
				double r6inv = r2inv * r2inv * r2inv;
				double force_divr= rinv * rsinv * r6inv * (12.0 * lj1  * r6inv - 6.0 * lj2);	
//				double pair_eng = r6inv * (lj1 * r6inv - lj2);
				virialXX += force_divr*dx*dx;
				virialYY += force_divr*dy*dy;
				virialZZ += force_divr*dz*dz;
				
				virialXY += force_divr*dx*dy;
				virialYZ += force_divr*dy*dz;
				virialZX += force_divr*dz*dx;				
				}
			id = m_list[id];	
			}
//----------compute with particles in surrrounding cells
		for(unsigned int im = 0; im <map.size(); im++)
			{
			int i = map[im].x;
			int j = map[im].y;
			int k = map[im].z;
			
	
			unsigned int jcell = cellid(ix+i, iy+j, iz+k, m_dim);
			id = m_head[jcell];
			while(id!=NO_INDEX)
				{
				double dx = px - pos[id].x;
				double dy = py - pos[id].y;
				double dz = pz - pos[id].z;
				double shiftx = 0.0;
				double shifty = 0.0;
				double shiftz = 0.0;
							
				if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
					{
					shiftx = rint(dx*Lxinv);
					shifty = rint(dy*Lyinv);
					shiftz = rint(dz*Lzinv);
								
					dx -=  Lx *shiftx;
					dy -=  Ly *shifty;				
					dz -=  Lz *shiftz;
					}
				double rsq = dx*dx + dy*dy + dz*dz;	
				double r = sqrt(rsq);
				double deltij=0.0;
				if (m_diameter)
					{
					double dj = diameter[id];
					deltij = (di + dj)/2.0 - 1.0;
					}
				double rs = r - deltij;				
				bool bexcluded =false; 
				if (m_bodyex&& my_body != NO_BODY)
					{
					unsigned int neigh_body = body[id];			
					bexcluded = (my_body == neigh_body);
					}
				int typ_pair = type[idx] * Ntypes + type[id];
				double lj1 = m_pparams[typ_pair].x;
				double lj2 = m_pparams[typ_pair].y;
				double rcut = m_pparams[typ_pair].z;

				if(r<rcut+deltij&&!m_mol->ifexclude(idx,id)&&!bexcluded)	
					{							
					double rinv = 1.0/r;
					double rsinv = 1.0/rs;
					double r2inv = rsinv * rsinv;
					double r6inv = r2inv * r2inv * r2inv;
					double force_divr= rinv * rsinv * r6inv * (12.0 * lj1  * r6inv - 6.0 * lj2);	
	//				double pair_eng = r6inv * (lj1 * r6inv - lj2);
					virialXX += force_divr*dx*dx;
					virialYY += force_divr*dy*dy;
					virialZZ += force_divr*dz*dz;
					
					virialXY += force_divr*dx*dy;
					virialYZ += force_divr*dy*dz;
					virialZX += force_divr*dz*dx;
					}
				id = m_list[id];	
				}	
			}
		}
		
//-----------------bond force
	std::vector<Bond> bonds = m_build->getBond();		
	for(unsigned int i=0; i< bonds.size();i++)
		{
		unsigned int a = bonds[i].a;
		unsigned int b = bonds[i].b;		
		unsigned int typ = bonds[i].id;
		vec bparam = m_bparams[typ];
		double dx = pos[a].x - pos[b].x;
		double dy = pos[a].y - pos[b].y;
		double dz = pos[a].z - pos[b].z;
		
		double shiftx = 0.0;
		double shifty = 0.0;
		double shiftz = 0.0;
							
		if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
			{
			shiftx = rint(dx*Lxinv);
			shifty = rint(dy*Lyinv);
			shiftz = rint(dz*Lzinv);
								
			dx -=  Lx *shiftx;
			dy -=  Ly *shifty;				
			dz -=  Lz *shiftz;
			}		
		 double rsq = dx*dx + dy*dy + dz*dz;
		 double r = sqrt(rsq);
		 double force_divr = bparam.x*(bparam.y/r-1.0);
		virialXX += force_divr*dx*dx;
		virialYY += force_divr*dy*dy;
		virialZZ += force_divr*dz*dz;
				
		virialXY += force_divr*dx*dy;
		virialYZ += force_divr*dy*dz;
		virialZX += force_divr*dz*dx;
		}
	for(unsigned int i=0;i<vel.size();i++)
		{
		vec veli = vel[i];
		double massi = mass[i];
		vvXX += veli.x*veli.x*massi;
		vvYY += veli.y*veli.y*massi;
		vvZZ += veli.z*veli.z*massi;

		vvXY += veli.x*veli.y*massi;
		vvYZ += veli.y*veli.z*massi;
		vvZX += veli.z*veli.x*massi;
		}
		
	double sigmaXX = (virialXX + vvXX)/vol;
	double sigmaYY = (virialYY + vvYY)/vol;	
	double sigmaZZ = (virialZZ + vvZZ)/vol;

	double sigmaXY = (virialXY + vvXY)/vol;
	double sigmaYZ = (virialYZ + vvYZ)/vol;	
	double sigmaZX = (virialZX + vvZX)/vol;
	
	double sigmavM = 0.5*(sigmaXX-sigmaYY)*(sigmaXX-sigmaYY) + 0.5*(sigmaYY-sigmaZZ)*(sigmaYY-sigmaZZ) + 0.5*(sigmaZZ-sigmaXX)*(sigmaZZ-sigmaXX);
	sigmavM += 3.0*sigmaXY*sigmaXY + 3.0*sigmaYZ*sigmaYZ + 3.0*sigmaZX*sigmaZX;
	sigmavM = sqrt(sigmavM);
	
	press.push_back(sigmaXX);
	press.push_back(sigmaYY);
	press.push_back(sigmaZZ);
	press.push_back(sigmaXY);
	press.push_back(sigmaYZ);
	press.push_back(sigmaZX);
	press.push_back(sigmavM);	
	std::string fname = m_build->getFilename();		
	m_file <<fname;
	for(unsigned int j =0; j< press.size(); j++)
		m_file <<"  "<<press[j]; 
	m_file <<"\n";
	m_Nf +=1;	
	}
//--- case 8
void Density::compute()
	{	
	unsigned int N = m_build->getNParticles();
	BoxSize box = m_build->getBox();
	std::vector< double > mass = m_build->getMass();
	if(mass.size()==0)
		{
        cerr << endl << "***Error! no input mass!" << endl << endl;
		throw runtime_error("Error Density::compute!");
		}	
	double tmass =0;
	for(unsigned int i=0; i< N; i++)
		tmass += mass[i];
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	double volume = Lx*Ly*Lz;
	double density = tmass/(volume*6.02*100);
	
	std::string fname = m_build->getFilename();		
	m_file <<fname;
	m_file <<"  "<<density<<" (g/cm^3)"; 
	m_file <<"\n";	
	}
//--- case 9
void Reimage::compute()
	{
	BoxSize box = m_build->getBox();
	std::vector<vec> pos0 = m_mol->getPos0();
	std::vector< double > mass = m_build->getMass();
	std::vector<vec> pos = m_build->getPos();
	std::vector<unsigned int> body = m_build->getBody();
	if(body.size()!=pos.size())
		m_body_keep=false;
	std::vector<Bond> bond = m_build->getBond();
	std::vector< std::string >bond_type_exchmap = m_build->getBondTypeMap();
	
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	
	double Lxinv = 0.0;
	double Lyinv = 0.0;
	double Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;		
	std::vector<vec_int> image;

	std::vector<std::string> name;
	std::vector<unsigned int> type = m_build->getType();
	std::vector< std::string > typemap =  m_build->getTypeMap();
	
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	std::vector< vec > center_per_mol;
	std::vector< unsigned int >massPerMol;
	std::vector<vec> pos0_first_mol; 
	std::vector<bool> first_mol_gate; 	
	center_per_mol.resize(mol_type_id.size());
	massPerMol.resize(mol_type_id.size());
	pos0_first_mol.resize(mol_type_id.size());
	first_mol_gate.resize(mol_type_id.size(),true);

	for (unsigned int i = 0; i< pos0.size(); i++)
		{
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			double px = pos0[i].x;
			double py = pos0[i].y;
			double pz = pos0[i].z;
			center_per_mol[molid].x += px;
			center_per_mol[molid].y += py;
			center_per_mol[molid].z += pz;
			massPerMol[molid] += 1;
			name.push_back(typemap[type[i]]);
			if(first_mol_gate[molid])
				{
				pos0_first_mol[molid] = pos0[i];
				first_mol_gate[molid]=false;
				}
			}
		else
			{
			if(typemap[type[i]] == m_target_type)
				name.push_back("FP");
			else
				name.push_back(typemap[type[i]]);				
			}
		}	
	if(m_keep_molecule_center_in_box)
		{		
		for (unsigned int i = 0; i< center_per_mol.size(); i++)
			{
			center_per_mol[i].x /= (double)massPerMol[i];
			center_per_mol[i].y /= (double)massPerMol[i];		
			center_per_mol[i].z /= (double)massPerMol[i];
			}
		}
	for (unsigned int i = 0; i< pos0.size(); i++)
		{
		unsigned int molid = mol_id_per_particle[i];

		int shiftx = rint((pos0[i].x-pos[i].x)*Lxinv);
		int shifty = rint((pos0[i].y-pos[i].y)*Lyinv);
		int shiftz = rint((pos0[i].z-pos[i].z)*Lzinv);		

		if(molid!=NO_INDEX)
			{				
			if(m_keep_molecule_center_in_box)
				{	
				shiftx -= rint((center_per_mol[molid].x+m_shiftx)*Lxinv);
				shifty -= rint((center_per_mol[molid].y+m_shifty)*Lyinv);
				shiftz -= rint((center_per_mol[molid].z+m_shiftz)*Lzinv);
				image.push_back(vec_int(shiftx, shifty, shiftz));					
				}
			else
				{
				if(m_shiftx!=0.0)
					shiftx -= rint((pos0_first_mol[molid].x+m_shiftx)*Lxinv);
				if(m_shifty!=0.0)				
					shifty -= rint((pos0_first_mol[molid].y+m_shifty)*Lyinv);
				if(m_shiftz!=0.0)				
					shiftz -= rint((pos0_first_mol[molid].z+m_shiftz)*Lzinv);					

				image.push_back(vec_int(shiftx, shifty, shiftz));				
				}
			}
		else
			{
			shiftx -= rint((pos0[i].x+m_shiftx)*Lxinv);
			shifty -= rint((pos0[i].y+m_shifty)*Lyinv);
			shiftz -= rint((pos0[i].z+m_shiftz)*Lzinv);
			image.push_back(vec_int(shiftx, shifty, shiftz));
			}
		}		

	std::string fname_in = m_build->getFilename();
	std::string fname_out = fname_in;	
	
	if(m_build->getObjectName()=="Dcdbuilder")
		fname_in=m_build->getFirstFilename();
	ifstream from(fname_in.c_str());
	
	string::size_type xp = fname_out.find("mst");
	string outs;
	if (xp!=fname_out.npos)
		outs = fname_out.replace(xp,xp+3, "reimage.mst");
	else
		outs = fname_out+".reimage.mst";

	ofstream to(outs.c_str());
	if(!from||!to) 
		cout<<"9.  Error!! reading or writing files failed!"<<endl;
	string line;
	bool imageout =false;
	bool posout =false;
	bool typeout =false;
	bool bondout =false;
	bool constraintout =false;	
	bool found_image =false;
	bool found_bond =false;	

	unsigned int count_image =0;
	unsigned int count_pos =0;	
	unsigned int count_type =0;
	unsigned int count_bond = 0;
	while(getline(from, line))
		{
		if(line.find("</configuration>")!= line.npos&&!found_image&&m_unwrap_molecule&&!m_remove_image)
			{
			to <<"<image num=\"" << image.size() << "\">" << "\n";
			for(unsigned int i=0;i<image.size();i++)
				{
				if(m_add_image_to_pos)
					to<<0 << " " << 0 << " "<< 0<< "\n";				
				else
					{
					if (m_body_keep)
						{
						unsigned int b = body[count_image];
						if(b!=NO_BODY)
							to<<0 << " " << 0 << " "<< 0 << "\n";
						else
							to<<image[i].x << " " << image[i].y << " "<< image[i].z << "\n";						
						}
					else
						to<<image[i].x << " " << image[i].y << " "<< image[i].z << "\n";	
					}
				}
			to<<"</image>"<<"\n";
			}
		if(line.find("</configuration>")!= line.npos&&!found_bond&&m_convert_constraints_to_bonds&&bond.size()>0)
			{
			to <<"<bond num=\"" << bond.size() << "\">" << "\n";
			for(unsigned int bi =0; bi<bond.size(); bi++)
				{
				bool out=true;
				if(m_remove_bond_cross_box)
					{
					float dx = 	pos[bond[bi].a].x - pos[bond[bi].b].x;
					float dy = 	pos[bond[bi].a].y - pos[bond[bi].b].y;
					float dz = 	pos[bond[bi].a].z - pos[bond[bi].b].z;
					if(fabs(dx)>Lx/2||fabs(dy)>Ly/2||fabs(dz)>Lz/2)
						out=false;
					}
				if(out)	
					to << bond_type_exchmap[bond[bi].id] << " " << bond[bi].a  << " " << bond[bi].b << "\n";
				}
			to<<"</bond>"<<"\n";
			}			
		if(line.find("<box")!= line.npos)
			{
			to << "<box " << "lx=\""<< Lx << "\" ly=\""<< Ly << "\" lz=\""<< Lz << "\"/>" << "\n";
			continue;
			}			
		if(line.find("</image")!= line.npos)
			imageout=false;
		if(line.find("</position")!= line.npos)
			posout=false;		
		if(line.find("</type")!= line.npos)
			typeout=false;
		if(line.find("</bond")!= line.npos&&(m_convert_constraints_to_bonds||m_remove_bond_cross_box))
			{
			if(count_bond<bond.size()&&m_convert_constraints_to_bonds)
				{
				for(unsigned int bi =count_bond; bi<bond.size(); bi++)
					{
					bool out=true;
					if(m_remove_bond_cross_box)
						{
						float dx = 	pos[bond[bi].a].x - pos[bond[bi].b].x;
						float dy = 	pos[bond[bi].a].y - pos[bond[bi].b].y;
						float dz = 	pos[bond[bi].a].z - pos[bond[bi].b].z;
						if(fabs(dx)>Lx/2||fabs(dy)>Ly/2||fabs(dz)>Lz/2)
							out=false;
						}
					if(out)	
						to << bond_type_exchmap[bond[bi].id] << " " << bond[bi].a  << " " << bond[bi].b << "\n";
					}
				}
			bondout=false;				
			}
		if(line.find("<constraint")!= line.npos&&m_convert_constraints_to_bonds)
			constraintout=true;			

		
		if (imageout&&m_unwrap_molecule)
			{
			if(m_remove_image||m_add_image_to_pos)
				to<<0 << " " << 0 << " "<< 0 << "\n";				
			else
				{
				if (m_body_keep)
					{
					unsigned int b = body[count_image];
					if(b!=NO_BODY)
						to<<0 << " " << 0 << " "<< 0 << "\n";
					else
						to<<image[count_image].x << " " << image[count_image].y << " "<< image[count_image].z << "\n";						
					}
				else
					to<<image[count_image].x << " " << image[count_image].y << " "<< image[count_image].z << "\n";
				}

			count_image += 1;
			}
		else if(posout)
			{
			if(m_add_image_to_pos)
				{
				if (m_body_keep)
					{
					unsigned int b = body[count_pos];
					if(b!=NO_BODY)
						to<<setiosflags(ios::fixed)<<setprecision(m_nprecision)<<setw(m_nprecision+m_nhead)<<pos[count_pos].x<<setw(m_nprecision+m_nhead)<<pos[count_pos].y<<setw(m_nprecision+m_nhead)<<pos[count_pos].z<< "\n";
					else
						to<<setiosflags(ios::fixed)<<setprecision(m_nprecision)<<setw(m_nprecision+m_nhead)<<pos[count_pos].x+double(image[count_pos].x)*Lx<<setw(m_nprecision+m_nhead)<<pos[count_pos].y+double(image[count_pos].y)*Ly<<setw(m_nprecision+m_nhead)<<pos[count_pos].z+double(image[count_pos].z)*Lz<< "\n";						
					}
				else
					to<<setiosflags(ios::fixed)<<setprecision(m_nprecision)<<setw(m_nprecision+m_nhead)<<pos[count_pos].x+double(image[count_pos].x)*Lx<<setw(m_nprecision+m_nhead)<<pos[count_pos].y+double(image[count_pos].y)*Ly<<setw(m_nprecision+m_nhead)<<pos[count_pos].z+double(image[count_pos].z)*Lz<< "\n";				
				}
			else		
				to<<setiosflags(ios::fixed)<<setprecision(m_nprecision)<<setw(m_nprecision+m_nhead)<<pos[count_pos].x<<setw(m_nprecision+m_nhead)<<pos[count_pos].y<<setw(m_nprecision+m_nhead)<<pos[count_pos].z<< "\n";
			count_pos += 1;			
			}
		else if (typeout&&m_label_free_particle)
			{
			to<<name[count_type] << "\n";
			count_type += 1;
			}
		else if (bondout&&(m_convert_constraints_to_bonds||m_remove_bond_cross_box))
			{
			bool out=true;
			if(m_remove_bond_cross_box)
				{
				float dx = 	pos[bond[count_bond].a].x - pos[bond[count_bond].b].x;
				float dy = 	pos[bond[count_bond].a].y - pos[bond[count_bond].b].y;
				float dz = 	pos[bond[count_bond].a].z - pos[bond[count_bond].b].z;
				if(fabs(dx)>Lx/2||fabs(dy)>Ly/2||fabs(dz)>Lz/2)
					out=false;
				}
			if(out)
				to << bond_type_exchmap[bond[count_bond].id] << " " << bond[count_bond].a  << " " << bond[count_bond].b << "\n";
			count_bond += 1;
			}
		else if (constraintout&&m_convert_constraints_to_bonds)
			{
			}				
		else
			to<<line<<"\n";

		
		if(line.find("<image") != line.npos)
			{
			imageout = true;
			found_image = true;
			}
		if(line.find("<position") != line.npos)
			{
			posout=true;
			}			
		if(line.find("<type") != line.npos)
			{
			typeout=true;
			}
		if(line.find("<bond") != line.npos&&(m_convert_constraints_to_bonds||m_remove_bond_cross_box))
			{
			bondout=true;
			found_bond = true;
			}	
		if(line.find("</constraint") != line.npos&&m_convert_constraints_to_bonds)
			{
			constraintout=false;
			}				
		}
	from.close();
	to.close();
	}
//--- case 10
void MSD::setParam()
	{
	std::vector<vec> pos = m_build->getPos();
	std::vector<vec_int> image = m_build->getImage();
	if(image.size()==0)
		{
        cerr << endl << "***Error! no input image!" << endl << endl;
		throw runtime_error("Error MSD::setParam!");
		}	
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	std::vector<unsigned int> mol_size = m_mol->getMolsize();
	m_pos_cm.resize(mol_type_id.size());
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	for (unsigned int i = 0; i< pos.size(); i++)
		{
		double px = pos[i].x+ double(image[i].x)*Lx;
		double py = pos[i].y+ double(image[i].y)*Ly;
		double pz = pos[i].z+ double(image[i].z)*Lz;
		m_pos_offset.push_back(vec(px,py,pz));
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			m_pos_cm[molid].x += px;
			m_pos_cm[molid].y += py;
			m_pos_cm[molid].z += pz;
			}
		}
	for (unsigned int i = 0; i< m_pos_cm.size(); i++)
		{
		m_pos_cm[i].x /= (double)mol_size[i];
		m_pos_cm[i].y /= (double)mol_size[i];		
		m_pos_cm[i].z /= (double)mol_size[i];	
		}		
	}

void MSD::compute()
	{
	if(m_Nf==0)
		setParam();
	std::vector<vec> pos = m_build->getPos();
	std::vector<vec_int> image = m_build->getImage();
	if(image.size()==0)
		{
        cerr << endl << "***Error! no input image!" << endl << endl;
		throw runtime_error("Error MSD!");
		}	
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	std::vector<unsigned int> type = m_build->getType();
	std::vector< std::string> type_map = m_build->getTypeMap();	
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	unsigned int n_kind_mol = m_mol->getKindNumofMol();
	std::vector<unsigned int > n_mol_per_kind = m_mol->getNmolperKind();
	std::vector<unsigned int> mol_size = m_mol->getMolsize();
	std::vector< std::string> free_particle_types = m_mol->getFreeParticleTypes();
	unsigned int n_types_free_particle = free_particle_types.size();
	std::vector< vec > center_per_mol;
	std::vector< double > msd_per_mol_kind;
	std::vector< double > msd_num;
	center_per_mol.resize(mol_type_id.size());
	msd_per_mol_kind.resize(n_kind_mol*2);
	msd_num.resize(n_kind_mol*2);
	std::vector<unsigned int > msd_num_free;
	std::vector< double > msd_free_particle;
	msd_num_free.resize(n_types_free_particle);
	msd_free_particle.resize(n_types_free_particle);
	
	for (unsigned int i = 0; i< pos.size(); i++)
		{
		double px = pos[i].x+ double(image[i].x)*Lx;
		double py = pos[i].y+ double(image[i].y)*Ly;
		double pz = pos[i].z+ double(image[i].z)*Lz;
		
		double dx = px - m_pos_offset[i].x;
		double dy = py - m_pos_offset[i].y;
		double dz = pz - m_pos_offset[i].z;
		double rsq = rsq_cal(m_direction, dx, dy, dz);

		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			center_per_mol[molid].x += px;
			center_per_mol[molid].y += py;		
			center_per_mol[molid].z += pz;

			unsigned int moltypeid = mol_type_id[molid];
			msd_per_mol_kind[moltypeid] += rsq;
			msd_num[moltypeid] += 1.0;
			}
		else
			{
			unsigned int typei = m_mol->getFreeParticleTypeId(type_map[type[i]]);
			if(typei==NO_INDEX)
				throw runtime_error("Error, MSD::compute for reading free particle type id");	
			msd_free_particle[typei] += rsq;
			msd_num_free[typei] +=1;
			}
		}
	for (unsigned int i = 0; i< center_per_mol.size(); i++)
		{
		center_per_mol[i].x /= (double)mol_size[i];
		center_per_mol[i].y /= (double)mol_size[i];		
		center_per_mol[i].z /= (double)mol_size[i];
		double dx = center_per_mol[i].x - m_pos_cm[i].x;
		double dy = center_per_mol[i].y - m_pos_cm[i].y;		
		double dz = center_per_mol[i].z - m_pos_cm[i].z;
		double rsq = rsq_cal(m_direction, dx, dy, dz);
		unsigned int moltypeid = mol_type_id[i];
		msd_per_mol_kind[moltypeid+n_kind_mol] += rsq;	
		msd_num[moltypeid+n_kind_mol] += 1.0;		
		}
	for (unsigned int i = 0; i< msd_per_mol_kind.size(); i++)
		{
		msd_per_mol_kind[i] /= (double)msd_num[i];
		}

	unsigned int NKmol = msd_per_mol_kind.size()/2;
	if(m_Nf==0)
		{
		m_file <<"Frame";
		for(unsigned int j =0; j< NKmol; j++)
			m_file <<"  Mol"<<j<<"_chain"<<"  Mol"<<j<<"_monomer";
		for(unsigned int j =0; j< n_types_free_particle; j++)
			m_file <<"  Free_"<<free_particle_types[j];
		m_file <<"\n"; 
		}
	m_file <<m_Nf;
	for(unsigned int j =0; j< NKmol; j++)
		m_file <<"   "<<msd_per_mol_kind[j+NKmol]<<"   "<<msd_per_mol_kind[j]; 
	for(unsigned int j =0; j< n_types_free_particle; j++)
		m_file <<"   "<<msd_free_particle[j]/double(msd_num_free[j]);
	m_file <<"\n"; 
	m_Nf += 1;	
	}
//--- case 11
void RDFCM::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "cm.rdf");
	else
		outs = fname+".cm.rdf";
	ofstream fp(outs.c_str());

	std::vector<vec> pos0 = m_mol->getPos0();
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();	
	std::vector<unsigned int> mol_size = m_mol->getMolsize();	
	std::vector< vec > center_per_mol;
	center_per_mol.resize(mol_type_id.size());

	if(mol_type_id.size()==0)
		{
        cerr << endl << "***Error! no molecules are detected!" << endl << endl;
		throw runtime_error("Error RDFCM::compute!");			
		}
		
	unsigned int ndimension = m_build->getNDimensions();	

	for (unsigned int i = 0; i< pos0.size(); i++)
		{
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			center_per_mol[molid].x += pos0[i].x;
			center_per_mol[molid].y += pos0[i].y;		
			center_per_mol[molid].z += pos0[i].z;
			}
		}
	for (unsigned int i = 0; i< center_per_mol.size(); i++)
		{
		center_per_mol[i].x /= (double)mol_size[i];
		center_per_mol[i].y /= (double)mol_size[i];		
		center_per_mol[i].z /= (double)mol_size[i];	
		}

	unsigned int N = center_per_mol.size();
//	unsigned int Ntypes = m_mol->getKindNumofMol();

	BoxSize box = m_build->getBox();
	std::vector<unsigned int> type =mol_type_id;

	float Lx = float (box.lx);
	float Ly = float (box.ly);		 
	float Lz = float (box.lz);		 
	float Lxinv = 0.0;
	float Lyinv = 0.0;
	float Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;	

	int nblocks = (int)ceil((float)N / (float)m_block_size);
	std::vector< double > r_DPD;
	std::vector< double > g;
	r_DPD.resize(m_maxbin);
	g.resize(m_maxbin);
	
 	float4 *hpos;
  	float4 *dpos;
	unsigned int* h_group;
	unsigned int* d_group;	
	unsigned int* scratch;
	unsigned int* d_scratch;
	unsigned int* d_gg;
	int nbytes = sizeof(float)*N*4;
	cudaHostAlloc(&hpos,nbytes,cudaHostAllocPortable);
	cudaHostAlloc(&h_group, sizeof(unsigned int)*N, cudaHostAllocPortable);	
	cudaHostAlloc(&scratch,sizeof(unsigned int)*nblocks*m_maxbin,cudaHostAllocPortable);		
    cudaMalloc(&dpos, nbytes);
	cudaMalloc(&d_group, sizeof(unsigned int)*N);
	cudaMalloc(&d_scratch, sizeof(unsigned int)*nblocks*m_maxbin);
	cudaMemset(d_scratch, 0, sizeof(unsigned int)*nblocks*m_maxbin);		
	cudaMalloc(&d_gg, sizeof(unsigned int)*nblocks*m_maxbin*m_block_size);
	cudaMemset(d_gg, 0, sizeof(unsigned int)*nblocks*m_maxbin*m_block_size);
		
	for(unsigned int i =0; i< N; i++)
		{
		hpos[i].x = center_per_mol[i].x;
		hpos[i].y = center_per_mol[i].y;
		hpos[i].z = center_per_mol[i].z;
		hpos[i].w = float(type[i]);
		h_group[i] = i;
		}	
	cudaMemcpy(dpos, hpos, nbytes, cudaMemcpyHostToDevice);
	cudaMemcpy(d_group, h_group, sizeof(unsigned int)*N, cudaMemcpyHostToDevice);
	float pi = 4.0*atan(1.0);
           
    float rmax = 0.5*Lx;
	if (m_rmax>0.0)
		rmax = m_rmax;	
	float rho  = (float)(N)*Lxinv*Lyinv*Lzinv;
	if(ndimension==2)
		rho = (float)(N)*Lxinv*Lyinv;	
	float delr = rmax/(float)m_maxbin;
	bool bytype = false;
	
	if (m_exclusion_mol)
		{
		d_mol_id_per_particle = m_mol->getMolIdPerParticleGPU();
		}
	if(m_exclusion_bond)
		{
		m_mol->bondExclude();
		m_exclusion_list=true;
		}
	if(m_exclusion_angle)
		{
		m_mol->angleExclude();
		m_exclusion_list=true;
		}		
	if (m_exclusion_list)
		{
		d_n_exclusion = m_mol->getNExclusionGPU();
		d_exclusion_list = m_mol->getExclusionListGPU();
		}					 

	gpu_compute_rdf(dpos,N,N,Lx,Ly,Lz,Lxinv,Lyinv,Lzinv,delr,d_gg,d_scratch,m_maxbin,d_group,
					d_n_exclusion,d_exclusion_list,d_mol_id_per_particle,m_exclusion_mol,
					m_exclusion_list,bytype,m_block_size);

	cudaMemcpy(scratch, d_scratch, sizeof(unsigned int)*nblocks*m_maxbin, cudaMemcpyDeviceToHost);

	for(unsigned int i =0; i< (unsigned int)nblocks;i++)
		{
		for(unsigned int j =0; j< m_maxbin;j++)
			g[j] += double(scratch[i*m_maxbin+j]);
		}

    double con = 4.0 * pi*rho/3.0;
	if(ndimension==2)
		con = pi*rho;
	for (unsigned int bin = 0; bin < m_maxbin; bin++ )
		{ 
	  	double rlower = (double)(bin)*delr;
	    double rupper = rlower + delr;
	    r_DPD[bin] = rlower + 0.5*delr;
	    double nid = con*(rupper*rupper*rupper-rlower*rlower*rlower);
		if(ndimension==2)
			nid = con*(rupper*rupper-rlower*rlower);
	    g[bin] /= (double)(N)*nid;
        fp<<r_DPD[bin]<<"   "<<g[bin]<<"\n";
		m_rdf[bin] += g[bin];
		}
	if(m_Nf==0)
		{
		for(unsigned int i=0; i<m_maxbin; i++ )
			m_r[i]=r_DPD[i];
		}
	fp.close();
	
	cudaFreeHost(hpos);
 	cudaFree(dpos);

	cudaFreeHost(scratch);
 	cudaFree(d_scratch);	
 	cudaFree(d_gg);
	m_Nf += 1;	
	} 
//--- case 12
void MSDCM::compute()
	{
	std::vector<vec> pos = m_build->getPos();
	std::vector<vec_int> image = m_build->getImage();
	if(image.size()==0)
		{
        cerr << endl << "***Error! no input image!" << endl << endl;
		throw runtime_error("Error MSDCM::compute!");
		}	
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();

	std::vector<unsigned int> mol_size = m_mol->getMolsize();
	std::vector< vec > center_per_mol;
	center_per_mol.resize(mol_type_id.size());
	for (unsigned int i = 0; i< pos.size(); i++)
		{
		double px = pos[i].x + double(image[i].x)*Lx;
		double py = pos[i].y + double(image[i].y)*Ly;
		double pz = pos[i].z + double(image[i].z)*Lz;
		
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			center_per_mol[molid].x += px;
			center_per_mol[molid].y += py;		
			center_per_mol[molid].z += pz;
			}
		}
	for (unsigned int i = 0; i< center_per_mol.size(); i++)
		{
		center_per_mol[i].x /= (double)mol_size[i];
		center_per_mol[i].y /= (double)mol_size[i];		
		center_per_mol[i].z /= (double)mol_size[i];	
		}
	m_pos_all.push_back(center_per_mol);
	if(m_Nf==0)
		{
		for(unsigned int i=0; i<mol_type_id.size(); i++)
			m_mol_type_id.push_back(mol_type_id[i]);
		m_n_kind_mol = m_mol->getKindNumofMol();				
		}
	m_Nf += 1;	
	}

MSDCM::~MSDCM() 
	{
	cout<<"12.  Computing msd of center mass ... "<<endl;
	std::vector< double > msdcm;
	std::vector< unsigned int > count;
	msdcm.resize(m_Nf*m_n_kind_mol);
	count.resize(m_n_kind_mol);
	
	for(unsigned int i=1; i<m_Nf; i++)
		{
		for(unsigned int c=0; c<m_n_kind_mol; c++)
			count[c] = 0;
		for(unsigned int j=i; j<m_Nf; j++)
			{
			unsigned int num_mol = m_pos_all[j].size();
			for(unsigned int k=0; k<num_mol; k++)
				{
				unsigned int moltypeid = m_mol_type_id[k];
				double dx = m_pos_all[j][k].x -m_pos_all[j-i][k].x;
				double dy = m_pos_all[j][k].y -m_pos_all[j-i][k].y;
				double dz = m_pos_all[j][k].z -m_pos_all[j-i][k].z;
				double rsq = rsq_cal(m_direction, dx, dy, dz);
				msdcm[i*m_n_kind_mol+moltypeid] += rsq;
				count[moltypeid] += 1;
				}
			}
		for(unsigned int c=0; c<m_n_kind_mol; c++)				
			msdcm[i*m_n_kind_mol+c] /= double(count[c]);
		}
	
	m_file <<"Frame";
	for(unsigned int j =0; j< m_n_kind_mol; j++)
		m_file <<"  Mol"<<j<<"_cm";
	m_file <<"\n";
	
	for(unsigned int i =0; i<m_Nf;i++)
		{
		m_file<<i;			
		for(unsigned int j =0; j< m_n_kind_mol; j++)
			m_file <<"  "<<msdcm[i*m_n_kind_mol+j];
		m_file <<"\n";
		}
	m_file.close();
	};	
	
//--- case 13
void Entanglement::setParam()
	{
	unsigned int Ntypes = m_build->getNParticleTypes();	
	m_rcutsq.resize(Ntypes*Ntypes);
	m_params.resize(Ntypes*Ntypes);
	std::vector< std::string > typemap =  m_build->getTypeMap();

	for(unsigned int typi=0; typi< Ntypes; typi++)
		{
		for(unsigned int typj=typi; typj< Ntypes; typj++)
			{
			cout<<"epsilon  sigma  alpha  rcut("<<typemap[typi]<<"  "<<typemap[typj]<<")"<<endl;   
			double epsilon, sigma, alpha, rcut;
			cin >> epsilon>>sigma>>alpha>>rcut;
			double lj1 = 4.0 * epsilon * pow(sigma, int(12));
			double lj2 = alpha * 4.0 * epsilon * pow(sigma, int(6));
			m_params[typi*Ntypes + typj] = vec(lj1, lj2, rcut*rcut);
			m_params[typj*Ntypes + typi] = vec(lj1, lj2, rcut*rcut);
			m_rcutsq[typi*Ntypes + typj] = rcut*rcut;
			m_rcutsq[typj*Ntypes + typi] = rcut*rcut;
			if(rcut>m_rcut_max)
				m_rcut_max = rcut;
			}
		}
	}

bool checkelement(std::vector<unsigned int >& a, std::vector<unsigned int >& b)
	{
	for(unsigned int i=0; i< a.size(); i++)
		{
		unsigned int ai = a[i];
		for(unsigned int j=0; j< b.size(); j++)
			{
			unsigned int bj=b[j];
			if(ai==bj)
				return true;
			}
		}
	return false;
	}
bool exist(std::vector<unsigned int >& a, unsigned int b)
	{
	for(unsigned int i=0; i< a.size(); i++)
		{
		unsigned int ai = a[i];
		if(ai==b)
			return true;
		}
	return false;
	}	
void Entanglement::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "interacts");
	else
		outs = fname+".interacts";
	ofstream fp(outs.c_str());
	
	if(m_Nf==0)
		setParam();	
//-calculate the center of mass of chains	
	std::vector<vec> pos0 = m_mol->getPos0();
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	std::vector< double > mass = m_build->getMass();
	std::vector< double > massPerMol;	
	std::vector< vec > center_per_mol;
	massPerMol.resize(mol_type_id.size());
	center_per_mol.resize(mol_type_id.size());
	for (unsigned int i = 0; i< pos0.size(); i++)
		{
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			double massi = mass[i];
			center_per_mol[molid].x += massi*pos0[i].x;
			center_per_mol[molid].y += massi*pos0[i].y;		
			center_per_mol[molid].z += massi*pos0[i].z;
			massPerMol[molid] += massi;
			}
		}
	for (unsigned int i = 0; i< center_per_mol.size(); i++)
		{
		center_per_mol[i].x /= (double)massPerMol[i];
		center_per_mol[i].y /= (double)massPerMol[i];		
		center_per_mol[i].z /= (double)massPerMol[i];	
		}	
//calculate the interaction list
	m_mol->computeList(m_rcut_max);
	std::vector<vec> pos = m_mol->getPos();
	std::vector<vec_int> map = m_mol->getMap();	
	std::vector<unsigned int> m_head = m_mol->getHead();
	std::vector<unsigned int> m_list = m_mol->getList();	
	vec m_width = m_mol->getWith();
	vec_uint m_dim = m_mol->getDim();	
	std::vector<unsigned int> type = m_build->getType();
//	std::vector<vec> vel = m_build->getVel();		
	unsigned int Ntypes = m_build->getNParticleTypes();
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);

	double Lxinv = 0.0;
	double Lyinv = 0.0;
	double Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;		
	std::vector<unsigned int> interaction_clist;  //interactions chain list
	std::vector<unsigned int> interaction_mlist; //interactions monomer list
	std::vector<bool> interaction_blist;	//interactions bool list
	std::vector<unsigned int> interaction_num;	
	unsigned int nmax=20;
	interaction_clist.resize(pos.size()*nmax);
	interaction_mlist.resize(pos.size()*nmax);
	interaction_blist.resize(pos.size()*nmax);
	interaction_num.resize(pos.size());
	
	std::vector<unsigned int> ents_list;	
	std::vector<unsigned int> epc_num; // number of ents per chain
	std::vector<unsigned int> mpe_num;	 // number of monomers per ent
	unsigned int nepc_max = 500;  //number of ents per chain
	unsigned int nmpe_max = 500;    //number of monomers per ent
	unsigned int Nchain = mol_type_id.size();
	ents_list.resize(Nchain*nepc_max *nmpe_max);
	epc_num.resize(Nchain);
	mpe_num.resize(Nchain*nepc_max);
	
//-----------------nb force
	for (unsigned int idx = 0; idx< pos.size(); idx++)
		{
	    double px = pos[idx].x;
		double py = pos[idx].y;
		double pz = pos[idx].z;

		int ix = int((px+0.5*Lx)/m_width.x);
		int iy = int((py+0.5*Ly)/m_width.y);
		int iz = int((pz+0.5*Lz)/m_width.z);
		unsigned int mymolid = mol_id_per_particle[idx];
//		unsigned int cid = cellid(ix, iy, iz, m_dim);
//----------compute with particles in same cell
		unsigned int id = m_list[idx];
		while(id!=NO_INDEX)
			{
			double dx = px - pos[id].x;
			double dy = py - pos[id].y;
			double dz = pz - pos[id].z;
			double shiftx = 0.0;
			double shifty = 0.0;
			double shiftz = 0.0;
							
			if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
				{
				shiftx = rint(dx*Lxinv);
				shifty = rint(dy*Lyinv);
				shiftz = rint(dz*Lzinv);
								
				dx -=  Lx *shiftx;
				dy -=  Ly *shifty;				
				dz -=  Lz *shiftz;
				}
			unsigned int typ_pair = type[idx] * Ntypes + type[id];
			double rcutsq = m_rcutsq[typ_pair];	
			
			double rsq = dx*dx + dy*dy + dz*dz;
			unsigned int molidj = mol_id_per_particle[id];
			if(rsq<rcutsq&&mymolid!=molidj)	
				{							
				unsigned int ni = interaction_num[idx];
				if(ni>=nmax)
					 throw runtime_error("Error, the number of interactions greater than nmax!");
				interaction_clist[idx*nmax+ni] = molidj;
				interaction_mlist[idx*nmax+ni] = id;				
				interaction_num[idx] = ni +1;
				}
			id = m_list[id];	
			}
//----------compute with particles in surrrounding cells
		for(unsigned int im = 0; im <map.size(); im++)
			{
			int i = map[im].x;
			int j = map[im].y;
			int k = map[im].z;
			
	
			unsigned int jcell = cellid(ix+i, iy+j, iz+k, m_dim);
			id = m_head[jcell];
			while(id!=NO_INDEX)
				{
				double dx = px - pos[id].x;
				double dy = py - pos[id].y;
				double dz = pz - pos[id].z;
				double shiftx = 0.0;
				double shifty = 0.0;
				double shiftz = 0.0;
							
				if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
					{
					shiftx = rint(dx*Lxinv);
					shifty = rint(dy*Lyinv);
					shiftz = rint(dz*Lzinv);
								
					dx -=  Lx *shiftx;
					dy -=  Ly *shifty;				
					dz -=  Lz *shiftz;
					}
				unsigned int typ_pair = type[idx] * Ntypes + type[id];
				double rcutsq = m_rcutsq[typ_pair];	
			
				double rsq = dx*dx + dy*dy + dz*dz;				
				unsigned int molidj = mol_id_per_particle[id];
				if(rsq<rcutsq&&mymolid!=molidj)	
					{							
					unsigned int ni = interaction_num[idx];
					if(ni>=nmax)
						 throw runtime_error("Error, the number of interactions greater than nmax!");
					interaction_clist[idx*nmax+ni] = molidj;
					interaction_mlist[idx*nmax+ni] = id;	
					interaction_num[idx] = ni +1;
					}
				id = m_list[id];	
				}	
			}
		}

	for(unsigned int j =0; j< interaction_num.size(); j++)
		{		
		unsigned int ni = interaction_num[j];		
		for(unsigned int k =0; k< nmax; k++)
			{
			if(k<ni)
				interaction_blist[j*nmax+k]=true;
			else
				interaction_blist[j*nmax+k]=false;				
			}
		}
// m_build ents list from interaction list
	for(unsigned int j =0; j< interaction_num.size(); j++)
		{
		unsigned int mychain = mol_id_per_particle[j];		
		unsigned int ni = interaction_num[j];
		for(unsigned int k=0;k<ni;k++)
			{
			if(interaction_blist[j*nmax+k])
				{
				unsigned int targetchain = interaction_clist[j*nmax+k];
				for(unsigned int l=j;l<interaction_num.size(); l++)
					{
					unsigned int nl=interaction_num[l];
					bool exist = false;
					unsigned int count =0;
					for(unsigned int m=0;m<nl; m++)
						{
						if(interaction_blist[l*nmax+m])
							count += 1;
						if(interaction_blist[l*nmax+m]&&interaction_clist[l*nmax+m]==targetchain)
							{
							unsigned int ne=epc_num[mychain];
							if(ne>=nepc_max)
								throw runtime_error("Error, the number of ents per chain greater than max!");								
							unsigned int nm=mpe_num[mychain*nepc_max+ne];
							if(nm>=nmpe_max)
								{
								cout<<nm<<" > "<<nmpe_max<<endl;
								throw runtime_error("Error, the number of monomers per ent greater than max!");	
								}
							ents_list[mychain*nepc_max*nmpe_max+ne*nmpe_max+nm]= l*nmax+m;
							mpe_num[mychain*nepc_max+ne]=nm+1;
							exist = true;
							interaction_blist[l*nmax+m] = false;
							}
						}
					if(!exist&&count>0)
						break;
					}
				epc_num[mychain] += 1;
				}
			}
		}
// combine some ents
////////////////////////////////////////////////////////////////////
	std::vector<unsigned int> ents_list_new;	
	std::vector<unsigned int> epc_num_new; // number of ents per chain
	std::vector<unsigned int> mpe_num_new;	 // number of monomers per ent
	ents_list_new.resize(ents_list.size());
	epc_num_new.resize(epc_num.size());
	mpe_num_new.resize(mpe_num.size());
	
	
	std::vector<bool> epc_blist;
	epc_blist.resize(mpe_num.size());
	for(unsigned int j=0;j<Nchain;j++)
		{
		unsigned int ne = epc_num[j];
		for(unsigned int k=0;k<nepc_max;k++)
			{
			if(k<ne)
				epc_blist[j*nepc_max+k]=true;
			else
				epc_blist[j*nepc_max+k]=false;
			}
		}
		
	for(unsigned int j=0;j<Nchain;j++)
		{
		unsigned int ne = epc_num[j];
		for(unsigned int k=0;k<ne;k++)
			{
			if(epc_blist[j*nepc_max+k])
				{
				std::vector<unsigned int> tempa_elist;
				unsigned int nm_a = mpe_num[j*nepc_max+k];
				for(unsigned int l=0;l<nm_a;l++)
					{
					unsigned int epos = ents_list[j*nepc_max*nmpe_max+k*nmpe_max+l];
					tempa_elist.push_back(interaction_mlist[epos]);
					}
				for(unsigned int m=k+1;m<ne;m++)
					{
					if(epc_blist[j*nepc_max+m])
						{
						std::vector<unsigned int> tempb_elist;
						unsigned int nm_b = mpe_num[j*nepc_max+m];
						for(unsigned int l=0;l<nm_b;l++)
							{
							unsigned int epos = ents_list[j*nepc_max*nmpe_max+m*nmpe_max+l];
							tempb_elist.push_back(interaction_mlist[epos]);
							}
						bool result = checkelement(tempa_elist, tempb_elist);
						if(result)
							{
							epc_blist[j*nepc_max+m]=false;
							for(unsigned int l=0;l<nm_b;l++)
								{
								unsigned int epos = ents_list[j*nepc_max*nmpe_max+m*nmpe_max+l];
								unsigned int nma = mpe_num[j*nepc_max+k];
								if(nma>=nmpe_max)
									throw runtime_error("Error, combine ents, the number of monomers per ent greater than max!");									
								ents_list[j*nepc_max*nmpe_max+k*nmpe_max+nma] = epos;
								mpe_num[j*nepc_max+k] = nma +1;
								}							
							for(unsigned int i=0;i<tempb_elist.size();i++)
								tempa_elist.push_back(tempb_elist[i]);
							m=k;
							}
						}
					}
				}
			}
		}
//re put
	for(unsigned int j=0;j<Nchain;j++)
		{
		unsigned int ne = epc_num[j];
		unsigned int ne_count=0;
		for(unsigned int k=0;k<ne;k++)
			{
			if(epc_blist[j*nepc_max+k])
				{
				unsigned int nm = mpe_num[j*nepc_max+k];				
				for(unsigned int l=0;l<nm;l++)
					{
					unsigned int epos = ents_list[j*nepc_max*nmpe_max+k*nmpe_max+l];
					ents_list_new[j*nepc_max*nmpe_max+ne_count*nmpe_max+l] = epos;
					}					
				mpe_num_new[j*nepc_max+ne_count]=nm;
				ne_count += 1;				
				}
			}
		epc_num_new[j] = ne_count;
		}
//////////////////////////////////////////////////////////////
//delete the same monomer in a ent
	for(unsigned int j=0;j<Nchain;j++)
		{
		unsigned int ne = epc_num_new[j];
		for(unsigned int k=0;k<ne;k++)
			{
			unsigned int nm = mpe_num_new[j*nepc_max+k];
			std::vector<unsigned int> temp_elist;
			unsigned int count =0;
			for(unsigned int l=0;l<nm;l++)
				{
				unsigned int epos = ents_list_new[j*nepc_max*nmpe_max+k*nmpe_max+l];
				unsigned int mid = interaction_mlist[epos];
				if(!exist(temp_elist,mid))
					{
					temp_elist.push_back(mid);
					ents_list[j*nepc_max*nmpe_max+k*nmpe_max+count] =epos;
					count += 1;
					}
				}					
			mpe_num[j*nepc_max+k]=count;
			}
		epc_num[j] = ne;
		}
		
// calculate ents force	
	
	for(unsigned int j=0;j<Nchain;j++)
		{
		unsigned int ne = epc_num[j];
		double cjx = center_per_mol[j].x;
		double cjy = center_per_mol[j].y;
		double cjz = center_per_mol[j].z;		
		for(unsigned int k=0;k<ne;k++)
			{
			unsigned int epos0 = ents_list[j*nepc_max*nmpe_max+k*nmpe_max+0];
			unsigned int ci = interaction_clist[epos0];

			double dcx= center_per_mol[ci].x - cjx;
			double dcy= center_per_mol[ci].y - cjy;
			double dcz= center_per_mol[ci].z - cjz;
			if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
				{
				double shiftx = rint(dcx*Lxinv);
				double shifty = rint(dcy*Lyinv);
				double shiftz = rint(dcz*Lzinv);
										
				dcx -=  Lx *shiftx;
				dcy -=  Ly *shifty;				
				dcz -=  Lz *shiftz;
				}				
			double lensq = dcx*dcx + dcy*dcy + dcz*dcz;
			double len = sqrt(lensq);
			double cuvx = dcx/len;           // unit vector of chain mass center in x direction
			double cuvy = dcy/len;
			double cuvz = dcz/len; 
			
			double forcex = 0.0;
			double forcey = 0.0;
			double forcez = 0.0;			
			unsigned int nm = mpe_num[j*nepc_max+k];	
			for(unsigned int l=0;l<nm;l++)
				{
				unsigned int epos = ents_list[j*nepc_max*nmpe_max+k*nmpe_max+l];
				unsigned int idx = interaction_mlist[epos];
				unsigned int ni = interaction_num[idx];
				double pix = pos[idx].x;
				double piy = pos[idx].y;
				double piz = pos[idx].z;				
				for(unsigned int i =0; i<ni; i++)
					{
					unsigned int mj =interaction_mlist[idx*nmax+i];
					unsigned int cj =interaction_clist[idx*nmax+i];	
					if(j==cj)
						{
						double dx = pix - pos[mj].x;
						double dy = piy - pos[mj].y;
						double dz = piz - pos[mj].z;
									
						if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
							{
							double shiftx = rint(dx*Lxinv);
							double shifty = rint(dy*Lyinv);
							double shiftz = rint(dz*Lzinv);
										
							dx -=  Lx *shiftx;
							dy -=  Ly *shifty;				
							dz -=  Lz *shiftz;
							}
						unsigned int typ_pair = type[idx] * Ntypes + type[mj];
						double lj1 = m_params[typ_pair].x;
						double lj2 = m_params[typ_pair].y;
						
						double rsq = dx*dx + dy*dy + dz*dz;	
						double r2inv = 1.0/rsq;
						double r6inv = r2inv * r2inv * r2inv;
						double force_divr= r2inv * r6inv * (12.0 * lj1  * r6inv - 6.0 * lj2);
						forcex += dx*force_divr;
						forcey += dy*force_divr;
						forcez += dz*force_divr;						
						}
					}
				}
			double force = 1.66*(forcex*cuvx + forcey*cuvy + forcez*cuvz);
			if(force>=m_fmax)
				{
				cerr << endl << "***Error, the force is !" << force << endl << endl;				
				throw runtime_error("Error, the force greater than the limist!");
				}
			else if(force>=0)
				{
				unsigned int i = (unsigned int)(fabs(force)/m_fdelt);
				m_fdistr[i] +=1;
				m_totalnum +=1;
				}
			}
		}		
		
//out put	
	unsigned int nents =0;
	std::vector<unsigned int> nm_ent;
	nm_ent.resize(nmpe_max);
	
	for(unsigned int j=0;j<Nchain;j++)
		{
		unsigned int ne = epc_num[j];
		nents += ne;
		fp <<" chain "<<j<<" with the number of ents "<<ne<<"\n";
		for(unsigned int k=0;k<ne;k++)
			{
			unsigned int nm = mpe_num[j*nepc_max+k];
			nm_ent[nm] += 1;
			fp <<" chain "<<j<<" ents "<<k<<" with the number of monomers "<<nm<<"\n";
			fp <<"cid:";	
			for(unsigned int l=0;l<nm;l++)
				{
				unsigned int epos = ents_list[j*nepc_max*nmpe_max+k*nmpe_max+l];
				fp <<" "<<interaction_clist[epos];
				}
			fp <<"\n";	
			fp <<"mid:";				
			for(unsigned int l=0;l<nm;l++)
				{
				unsigned int epos = ents_list[j*nepc_max*nmpe_max+k*nmpe_max+l];
				fp <<" "<<interaction_mlist[epos];
				}
			fp <<"\n";
			}
		}
	cout<<"Num of ents per chain :"<<m_totalnum/Nchain<<endl;
	cout<<"Num of interactions per chain :"<<nents/Nchain<<endl;	
	fp<<"out put interactions"<<"\n";
	for(unsigned int j =0; j< interaction_num.size(); j++)
		{
		unsigned int ni = interaction_num[j];
		fp <<mol_id_per_particle[j]<<"  "<<j<<" :"; 
		for(unsigned int k =0; k< ni; k++)
			{
			fp <<"  "<<interaction_clist[j*nmax+k]; 
			}
		// fp <<" | ";	
		// for(unsigned int k =0; k< ni; k++)
			// {
			// fp <<"  "<<interaction_mlist[j*nmax+k]; 
			// }			
		fp <<"\n";			
		}
	fp<<" monomers in ents statistics" <<"\n";	
	for(unsigned int i=0; i<nmpe_max;i++)
		{
		if(nm_ent[i]>0)
			fp<<i<<" "<< nm_ent[i]<<"\n";
		}
	fp.close();
	m_Nf +=1;	
	}	
//--- case 14
typedef float2 cuFloatComplex;
 extern "C"{ 
cudaError_t gpu_compute_domainsize(float *x,
							float *y,
                            float *z,
							float *w,
                            unsigned int Num,
							int Lmax,
							int Mmax,
							int Nmax,							
							float Lxinv,
							float Lyinv,
							float Lzinv,
							int Kmax,
							float TWOPI,							
							unsigned int Ntypes,
							cuFloatComplex *ss,
							unsigned int block_size);
} 

union floatint
    {
    float f;        //!< Scalar to read/write
    int i;  //!< int to read/write at the same memory location
    };

void STRFAC::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "strf");
	else
		outs = fname+".strf";
	ofstream fp(outs.c_str());

	unsigned int Num = m_build->getNParticles();
	unsigned int Ntypes = m_build->getNParticleTypes();
    std::vector< std::string> type_map = m_build->getTypeMap();

	BoxSize box = m_build->getBox();
	std::vector<vec> pos = m_build->getPos();

	std::vector<unsigned int> type = m_build->getType();

	unsigned int ndimension = m_build->getNDimensions();	

	if(ndimension==2||m_2D)
		m_direction="XY";

	float Lx = float (box.lx);
	float Ly = float (box.ly);		 
	float Lz = float (box.lz);
	if(m_L==0.0)
		{
		m_L = min(min(Lx, Ly), Lz);
		
		if(m_direction=="X")
			m_L = Lx;
		else if(m_direction=="Y")
			m_L = Ly;							
		else if(m_direction=="Z")
			m_L = Lz;	
		else if(m_direction=="XY")
			m_L = min(Lx, Ly);	
		else if(m_direction=="XZ")
			m_L = min(Lx, Lz);	
		else if(m_direction=="YZ")
			m_L = min(Ly, Lz);
		}

	if(m_L==0)
		{
        cerr << endl << "***Error! m_L = 0.0!" << endl << endl;
		throw runtime_error("Error STRFAC::compute!");
		}

	Lx = m_L;
	Ly = m_L;		 
	Lz = m_L;		

	float Lxinv = 1.0 / Lx;
	float Lyinv = 1.0 / Ly;
	float Lzinv = 1.0 / Lz;
	
	if(m_Qmax!=0)
		m_Kmax = int(m_Qmax*m_L/(M_PI*2.0));	

	m_LKmax=m_Kmax;
	m_MKmax=m_Kmax;
	m_NKmax=m_Kmax;
	
	if(m_2D)
		m_NKmax=0;
	
	int Ksqmax = m_Kmax*m_Kmax;
	int NI = Ntypes*(Ntypes+1)/2;
	
	int Lmax = m_LKmax+1;
	int Mmax = 2*m_MKmax+1;
	int Nmax = 2*m_NKmax+1;
	
	if(m_Nf==0)
		{
		m_sqreal.resize(NI*Ksqmax);
		m_sqimage.resize(NI*Ksqmax);
		m_type_map=type_map;
		m_Ntypes=Ntypes;

		if(m_2D)
			{
			m_sqreal2D.resize(NI*Lmax);
			m_sqimage2D.resize(NI*Lmax);

			for(unsigned int i=0; i<(unsigned int)NI*Lmax; i++)
				{
				m_sqreal2D[i].resize(NI*Mmax);
				m_sqimage2D[i].resize(NI*Mmax);			
				}
			}
		}	
 	float *x;
	float *y;
	float *z;
	float *w;
  	float *dx;
	float *dy;
	float *dz;
	float *dw;

	cuFloatComplex *ss;
	cuFloatComplex *d_ss;		
	int nbytes = sizeof(float)*Num;
	cudaHostAlloc(&x,nbytes,cudaHostAllocPortable);
	cudaHostAlloc(&y,nbytes,cudaHostAllocPortable);	
	cudaHostAlloc(&z,nbytes,cudaHostAllocPortable);	
	cudaHostAlloc(&w,nbytes,cudaHostAllocPortable);	
	cudaHostAlloc(&ss,sizeof(cuFloatComplex)*Lmax*Mmax*Nmax*Ntypes,cudaHostAllocPortable);		
    cudaMalloc(&dx, nbytes);
	cudaMalloc(&dy, nbytes);
	cudaMalloc(&dz, nbytes);
	cudaMalloc(&dw, nbytes);
	cudaMalloc(&d_ss, sizeof(cuFloatComplex)*Lmax*Mmax*Nmax*Ntypes);

	for(unsigned int i =0; i< Num; i++)
		{
		x[i] = pos[i].x;
		y[i] = pos[i].y;
		z[i] = pos[i].z;
		floatint fi;
		fi.i = type[i];
		w[i] = fi.f;
//		cout<<"type"<<w[i]<<endl;
		}	
//		cout<<"Num"<<Num<<endl;
	  cudaMemcpy(dx, x, nbytes, cudaMemcpyHostToDevice);		
	  cudaMemcpy(dy, y, nbytes, cudaMemcpyHostToDevice);		
	  cudaMemcpy(dz, z, nbytes, cudaMemcpyHostToDevice);		
	  cudaMemcpy(dw, w, nbytes, cudaMemcpyHostToDevice);
	
	if((unsigned int)(Lmax*Mmax*Nmax)>65535*m_block_size)
		m_block_size=512;
	
	if((unsigned int)(Lmax*Mmax*Nmax)>65535*m_block_size)
		{
        cerr << endl << "***Error! the number of q vectors "<<Lmax*Mmax*Nmax<< " larger than uplimited!" << endl << endl;
		throw runtime_error("Error STRFAC::compute()!");
		}	
	
	 gpu_compute_domainsize(dx,dy,dz,dw,Num,Lmax,Mmax,Nmax,Lxinv,Lyinv,Lzinv,m_Kmax,M_PI*2.0,Ntypes,d_ss,m_block_size);
	 
	 cudaMemcpy(ss, d_ss, sizeof(cuFloatComplex)*Lmax*Mmax*Nmax*Ntypes, cudaMemcpyDeviceToHost);	
	  // for(unsigned int i=0;i<Lmax*Mmax*Nmax*Ntypes;i++)
		  // cout<<"ss"<<ss[i].x<<" "<<ss[i].y<<endl;
	
	cuFloatComplex sk[Ksqmax*NI];

	unsigned int ick[Ksqmax];
	
	for(unsigned int i =0; i< (unsigned int)Ksqmax; i++)
		{
		ick[i] = 0;
		for(unsigned int t =0; t< (unsigned int)NI; t++)
			{
			sk[i+t*Ksqmax].x = 0.0;
			sk[i+t*Ksqmax].y = 0.0;		
			}
		}
	
	cuFloatComplex **sk2D;
	sk2D=new cuFloatComplex*[Lmax*NI]; 	

	for(unsigned int i=0; i<(unsigned int)Lmax*NI; i++)  
		sk2D[i]=new cuFloatComplex[Mmax*NI];
	
	for(unsigned int i =0; i< (unsigned int)Lmax*NI; i++)
		{
		for(unsigned int j =0; j< (unsigned int)Mmax*NI; j++)
			{
			sk2D[i][j].x = 0.0;
			sk2D[i][j].y = 0.0;		
			}
		}	 		
		
			
	int mmin=m_MKmax;
	int nmin=m_NKmax+1;
	for(unsigned int l =0; l< (unsigned int)Lmax; l++)
		{
		for(unsigned int m =mmin; m< (unsigned int)Mmax; m++)
			{
			for(unsigned int n =nmin; n< (unsigned int)Nmax; n++)
				{
				int k = l + m*Lmax + n*Lmax*Mmax;
				int ll = l;
				int mm = m - m_MKmax;
				int nn = n - m_NKmax;
				int kk = ll*ll +mm*mm +nn*nn -1;
				if(kk<Ksqmax||m_2D)
					{
					bool cal = false;
					if(m_direction=="X"&&mm==0&&nn==0)
						cal=true;
					else if(m_direction=="Y"&&ll==0&&nn==0)
						cal=true;
					else if(m_direction=="Z"&&ll==0&&mm==0)
						cal=true;
					else if(m_direction=="XY"&&nn==0)
						cal=true;
					else if(m_direction=="XZ"&&mm==0)
						cal=true;
					else if(m_direction=="YZ"&&ll==0)
						cal=true;
					else if(m_direction=="XYZ")
						cal=true;						
					if(cal)
						{						
						ick[kk] += 1;
						unsigned int icn =0;
						for(unsigned int typi =0; typi < Ntypes; typi++)
							{
	//					 cout<<typi<<" "<<ll<<" "<<mm<<" "<<nn<<" "<<ss[typi+k*Ntypes].x<<" "<<ss[typi+k*Ntypes].y<<endl;
							for(unsigned int typj =typi; typj < Ntypes; typj++)
								{
								cuFloatComplex mul = cuCmulf(cuConjf(ss[typi+k*Ntypes]),ss[typj+k*Ntypes]);
								// if(l==48&&typi==0&&typj==0)
									// cout<<mul.x<<" "<<mul.y<<endl;
								if(m_2D)
									sk2D[l+icn*(unsigned int)Lmax][m+icn*(unsigned int)Mmax] = mul;
								else
									sk[kk+icn*Ksqmax] = cuCaddf(sk[kk+icn*Ksqmax],mul);
								icn +=1;								
								}
							}	
						}
					}
				}
			  nmin=0;	
			}
		  mmin =0; 	
		}
	
	for(unsigned int i =0; i< (unsigned int)NI; i++)
		{	
		for(unsigned int j =0; j<(unsigned int)Ksqmax; j++)
			{	
			unsigned int id = i*Ksqmax + j;
			if(ick[j]!=0)
				{
				sk[id].x /= float(Num*ick[j]);
				sk[id].y /= float(Num*ick[j]);
				}
//				cout<<"sk"<<id<<" "<<sk[id].x<<" "<<sk[id].y<<endl;			
			}
		}

	for(unsigned int i =0; i< (unsigned int)Lmax*NI; i++)
		{
		for(unsigned int j =0; j< (unsigned int)Mmax*NI; j++)
			{
			sk2D[i][j].x /= float(Num);
			sk2D[i][j].y /= float(Num);		
			}
		}	 		

	unsigned int icn =0;		
	for(unsigned int typi =0; typi < Ntypes; typi++)
		{
		for(unsigned int typj =typi; typj < Ntypes; typj++)
			{
			fp<<type_map[typi]<<" "<<type_map[typj]<<endl;
			if(m_2D)
				{
				for(unsigned int i =0; i<(unsigned int)Lmax; i++)
					{
					double qi =double(i)*M_PI*2.0/m_L;
					unsigned int idi = i+icn*Lmax;
					for(unsigned int j =0; j<(unsigned int)Mmax; j++)
						{
						unsigned int idj = j+icn*Mmax;							
						double qj =double(int(j)-int(m_MKmax))*M_PI*2.0/m_L;
						if(i==0&&int(j)<m_MKmax)
							idj = 2*m_MKmax-j+icn*Mmax;
						fp<<qi<<"   "<<qj<<"   "<<sk2D[idi][idj].x<<"   "<<sk2D[idi][idj].y<<endl;

						m_sqreal2D[idi][idj] +=double(sk2D[idi][idj].x);
						m_sqimage2D[idi][idj] +=double(sk2D[idi][idj].y);
						}
					}
				for(unsigned int i =1; i<(unsigned int)Lmax; i++)
					{
					double qi =double(i)*M_PI*2.0/m_L;
					unsigned int idi = i+icn*Lmax;
					for(unsigned int j =0; j<(unsigned int)Mmax; j++)
						{
						unsigned int idj = j+icn*Mmax;							
						double qj =double(int(j)-int(m_MKmax))*M_PI*2.0/m_L;
						fp<<-qi<<"   "<<-qj<<"   "<<sk2D[idi][idj].x<<"   "<<sk2D[idi][idj].y<<endl;
						}
					}						
				}
			else
				{
				for(unsigned int j =0; j<(unsigned int)Ksqmax; j++)
					{
					unsigned int id = icn*Ksqmax + j;
					double q =sqrt(double(j+1))*M_PI*2.0/m_L;
					if(sk[id].x!=0.0||sk[id].y!=0.0)
						fp<<q<<"   "<<sk[id].x<<"   "<<sk[id].y<<endl;
					m_sqreal[id] +=double(sk[id].x);
					m_sqimage[id] +=double(sk[id].y);
					}				
				}
			icn +=1;				
			}
		}

	fp.close();
	m_Nf += 1;	
	cudaFreeHost(x);
	cudaFreeHost(y);
	cudaFreeHost(z);
	cudaFreeHost(w);
 		cudaFree(dx);
 		cudaFree(dy);
 		cudaFree(dz);
 		cudaFree(dw);

	cudaFreeHost(ss);
 		cudaFree(d_ss);	
	} 
//--- case 15
void DOMAINSIZE::compute()
	{
	unsigned int Num = m_build->getNParticles();
	unsigned int Ntypes = m_build->getNParticleTypes();
    std::vector< std::string> type_map = m_build->getTypeMap();
	
	BoxSize box = m_build->getBox();
	std::vector<vec> pos = m_build->getPos();
	std::vector<unsigned int> type = m_build->getType();
	unsigned int ndimension = m_build->getNDimensions();
	
	float Lx = float (box.lx);
	float Ly = float (box.ly);		 
	float Lz = float (box.lz);

	float lmin = min(min(Lx, Ly), Lz);
	if(ndimension==2)
		lmin = min(Lx, Ly);
	
	Lx = lmin;
	Ly = lmin;		 
	Lz = lmin;
	
	float Lxinv = 0.0;
	float Lyinv = 0.0;
	float Lzinv = 0.0;
	
	if(Lx!=0.0)
		Lxinv = 1.0 / Lx;
	if(Ly!=0.0)
		Lyinv = 1.0 / Ly;
	if(Lz!=0.0)
		Lzinv = 1.0 / Lz;	

	int Kmax = int(m_Kmax);
	int Ksqmax = Kmax*Kmax;
	int NI = Ntypes*(Ntypes+1)/2;
	int Lmax = Kmax+1;
	int Mmax = 2*Kmax+1;
	int Nmax = 2*Kmax+1;
	

 	float *x;
	float *y;
	float *z;
	float *w;
  	float *dx;
	float *dy;
	float *dz;
	float *dw;

	cuFloatComplex *ss;
	cuFloatComplex *d_ss;		
	int nbytes = sizeof(float)*Num;
	cudaHostAlloc(&x,nbytes,cudaHostAllocPortable);
	cudaHostAlloc(&y,nbytes,cudaHostAllocPortable);	
	cudaHostAlloc(&z,nbytes,cudaHostAllocPortable);	
	cudaHostAlloc(&w,nbytes,cudaHostAllocPortable);	
	cudaHostAlloc(&ss,sizeof(cuFloatComplex)*Lmax*Mmax*Nmax*Ntypes,cudaHostAllocPortable);		
    cudaMalloc(&dx, nbytes);
	cudaMalloc(&dy, nbytes);
	cudaMalloc(&dz, nbytes);
	cudaMalloc(&dw, nbytes);
	cudaMalloc(&d_ss, sizeof(cuFloatComplex)*Lmax*Mmax*Nmax*Ntypes);
	
	for(unsigned int i =0; i< Num; i++)
		{
		x[i] = pos[i].x;
		y[i] = pos[i].y;
		z[i] = pos[i].z;
		floatint fi;
		fi.i = type[i];
		w[i] = fi.f;
//		cout<<"type"<<w[i]<<endl;
		}	
//		cout<<"Num"<<Num<<endl;
	  cudaMemcpy(dx, x, nbytes, cudaMemcpyHostToDevice);		
	  cudaMemcpy(dy, y, nbytes, cudaMemcpyHostToDevice);		
	  cudaMemcpy(dz, z, nbytes, cudaMemcpyHostToDevice);		
	  cudaMemcpy(dw, w, nbytes, cudaMemcpyHostToDevice);
	
	 gpu_compute_domainsize(dx,dy,dz,dw,Num,Lmax,Mmax,Nmax,Lxinv,Lyinv,Lzinv,Kmax,M_PI*2.0,Ntypes,d_ss,m_block_size);
	 
	 cudaMemcpy(ss, d_ss, sizeof(cuFloatComplex)*Lmax*Mmax*Nmax*Ntypes, cudaMemcpyDeviceToHost);	
	  // for(unsigned int i=0;i<Lmax*Mmax*Nmax*Ntypes;i++)
		  // cout<<"ss"<<ss[i].x<<" "<<ss[i].y<<endl;
	
	cuFloatComplex sk[Ksqmax*NI];
	unsigned int ick[Ksqmax];
	for(unsigned int i =0; i< (unsigned int)Ksqmax; i++)
		{
		ick[i] = 0;
		for(unsigned int t =0; t< (unsigned int)NI; t++)
			{
			sk[i+t*Ksqmax].x = 0.0;
			sk[i+t*Ksqmax].y = 0.0;		
			}
		}		
	int mmin=Kmax;
	int nmin=Kmax+1;
	for(unsigned int l =0; l< (unsigned int)Lmax; l++)
		{
		for(unsigned int m =mmin; m< (unsigned int)Mmax; m++)
			{
			for(unsigned int n =nmin; n< (unsigned int)Nmax; n++)
				{
				int k = l + m*Lmax + n*Lmax*Mmax;
				int ll = l;
				int mm = m - Kmax;
				int nn = n - Kmax;
				int kk = ll*ll +mm*mm +nn*nn -1;
				if(kk<Ksqmax)
					{
					bool cal = false;
					if(ndimension==3)
						cal=true;
					else if(ndimension==2&&nn==0)
						cal=true;
					else if(ndimension==1&&mm==0&&nn==0)
						cal=true;
					if(cal)
						{
						ick[kk] += 1;
						unsigned int icn =0;
						for(unsigned int typi =0; typi < Ntypes; typi++)
							{
	//					 cout<<typi<<" "<<ll<<" "<<mm<<" "<<nn<<" "<<ss[typi+k*Ntypes].x<<" "<<ss[typi+k*Ntypes].y<<endl;
							for(unsigned int typj =typi; typj < Ntypes; typj++)
								{
								cuFloatComplex mul = cuCmulf(cuConjf(ss[typi+k*Ntypes]),ss[typj+k*Ntypes]);
								sk[kk+icn*Ksqmax] = cuCaddf(sk[kk+icn*Ksqmax],mul);
								icn +=1;								
								}
							}
						}						
					}
				}
			  nmin=0;	
			}
		  mmin =0; 	
		}

	for(unsigned int i =0; i< (unsigned int)NI; i++)
		{	
		for(unsigned int j =0; j<(unsigned int)Ksqmax; j++)
			{	
			unsigned int id = i*Ksqmax + j;
			if(ick[j]!=0)
				{
				sk[id].x /= float(Num*ick[j]);
				sk[id].y /= float(Num*ick[j]);
				}
//				cout<<"sk"<<id<<" "<<sk[id].x<<" "<<sk[id].y<<endl;			
			}
		}
		
	std::string fname = m_build->getFilename();	
	m_file <<fname<<" ";		
	unsigned int icn =0;
	unsigned int cutoff = (unsigned int)(m_qc*Lx*m_qc*Lx);
	for(unsigned int typi =0; typi < Ntypes; typi++)
		{
		for(unsigned int typj =typi; typj < Ntypes; typj++)
			{
			m_file<<type_map[typi]<<"-"<<type_map[typj]<<" ";
			float sq = 0.0;
			float sqq = 0.0;
			for(unsigned int j =0; j<(unsigned int)Ksqmax; j++)
				{
				unsigned int id = icn*Ksqmax + j;
				if(j<cutoff)
					{
					sq += sk[id].x;
					sqq += sk[id].x*sqrt(float(j+1))*M_PI*2.0/Lx;
					}
	//			d<<i<<" "<<j<<" "<<sqreal[id]<<" "<<sqimage[id]<<endl;			
				}
			m_file<<sq*M_PI*2.0/sqq<<" ";
			icn +=1;				
			}
		}
	m_file <<"\n";
	
	m_Nf += 1;	
	cudaFreeHost(x);
	cudaFreeHost(y);
	cudaFreeHost(z);
	cudaFreeHost(w);
 		cudaFree(dx);
 		cudaFree(dy);
 		cudaFree(dz);
 		cudaFree(dw);

	cudaFreeHost(ss);
 		cudaFree(d_ss);	
	} 
//--- case 16
void DSTRFAC::setParam()
	{
	std::vector<vec> pos = m_build->getPos();
	std::vector<vec_int> image = m_build->getImage();
	if(image.size()==0)
		{
        cerr << endl << "***Error! no input image!" << endl << endl;
		throw runtime_error("Error DSTRFAC::setParam!");
		}	
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	std::vector<unsigned int> mol_size = m_mol->getMolsize();
	m_pos_cm.resize(mol_type_id.size());
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	for (unsigned int i = 0; i< pos.size(); i++)
		{
		double px = pos[i].x + double(image[i].x)*Lx;
		double py = pos[i].y + double(image[i].y)*Ly;
		double pz = pos[i].z + double(image[i].z)*Lz;
		m_pos_offset.push_back(vec(px,py,pz));
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			m_pos_cm[molid].x += px;
			m_pos_cm[molid].y += py;
			m_pos_cm[molid].z += pz;
			}
		}
	for (unsigned int i = 0; i< m_pos_cm.size(); i++)
		{
		m_pos_cm[i].x /= (double)mol_size[i];
		m_pos_cm[i].y /= (double)mol_size[i];		
		m_pos_cm[i].z /= (double)mol_size[i];	
		}	
	float nsq = m_q/(2.0*M_PI);
	float nn = nsq*nsq;
	
	if(m_Kmax.x==0&&m_Kmax.y==0&&m_Kmax.z==0)
		m_Kmax = vec_int(int(Lx), int(Ly), int(Lz));
	
	for(int i=0;i<int(m_Kmax.x);i++)
		{
		for(int j=0;j<int(m_Kmax.y);j++)
			{
			for(int k=0;k<int(m_Kmax.z);k++)
				{
				float mm = float(i*i)/(Lx*Lx) +float(j*j)/(Ly*Ly) +float(k*k)/(Lz*Lz);
				if(fabs(nn-mm)/nn<0.01)
					{
					//cout<<nn<<" "<<mm<<" "<<fabs(nn-mm)/nn<<endl;
					m_qvec.push_back(vec_int(i,j,k));
					m_qvec.push_back(vec_int(-i,j,k));
					m_qvec.push_back(vec_int(i,-j,k));	
					m_qvec.push_back(vec_int(-i,-j,k));						
					}
				}
			}
		}
	cout<<"16.  The number of q vector is "<<m_qvec.size()<<endl;
	if(m_qvec.size()==0)
		{
        cerr << endl << "***Error! no qvec specified!" << endl << endl;
		throw runtime_error("Error DSTRFAC::setParam!");
		}		
	}
void DSTRFAC::compute()
	{
	if(m_Nf==0)
		setParam();
	std::vector<vec> pos = m_build->getPos();
	std::vector<vec_int> image = m_build->getImage();
	if(image.size()==0)
		{
        cerr << endl << "***Error! no input image!" << endl << endl;
		throw runtime_error("Error DSTRFAC!");
		}	
	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);
	std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();
	std::vector<unsigned int> type = m_build->getType();	
	std::vector<unsigned int > mol_type_id = m_mol->getMolTypeId();
	unsigned int n_kind_mol = m_mol->getKindNumofMol();
	std::vector<unsigned int > n_mol_per_kind = m_mol->getNmolperKind();
	std::vector<unsigned int> mol_size = m_mol->getMolsize();
	std::vector< vec > center_per_mol;
	std::vector< cuFloatComplex > DSTRFPerMolKind;
	std::vector< double > DSTRFnum;
	center_per_mol.resize(mol_type_id.size());
	DSTRFPerMolKind.resize(n_kind_mol*2);
	DSTRFnum.resize(n_kind_mol*2);
	unsigned int DSTRFnumfree=0;
	cuFloatComplex DSTRFfree;
	DSTRFfree.x=0.0;
	DSTRFfree.y=0.0;

	for (unsigned int i = 0; i< pos.size(); i++)
		{
		double px = pos[i].x+ double(image[i].x)*Lx;
		double py = pos[i].y+ double(image[i].y)*Ly;
		double pz = pos[i].z+ double(image[i].z)*Lz;
		
		double dx = px - m_pos_offset[i].x;
		double dy = py - m_pos_offset[i].y;
		double dz = pz - m_pos_offset[i].z;	
		
		unsigned int molid = mol_id_per_particle[i];
		if(molid!=NO_INDEX)
			{
			center_per_mol[molid].x += px;
			center_per_mol[molid].y += py;		
			center_per_mol[molid].z += pz;

			unsigned int moltypeid = mol_type_id[molid];			
			for (unsigned int j = 0; j< m_qvec.size(); j++)
				{
				vec_int qvec = m_qvec[j];
				float theta = 2*M_PI*(float(qvec.x)*dx/Lx + float(qvec.y)*dy/Ly + float(qvec.z)*dz/Lz);
				cuFloatComplex expikr;
				expikr.x = cosf(theta);
				expikr.y = sinf(theta);
				DSTRFPerMolKind[moltypeid] = cuCaddf(DSTRFPerMolKind[moltypeid],expikr);
				}
			DSTRFnum[moltypeid] += 1.0;
			}	
		else
			{
			for (unsigned int j = 0; j< m_qvec.size(); j++)
				{
				vec_int qvec = m_qvec[j];
				float theta = 2*M_PI*(float(qvec.x)*dx/Lx + float(qvec.y)*dy/Ly + float(qvec.z)*dz/Lz);
				cuFloatComplex expikr;
				expikr.x = cosf(theta);
				expikr.y = sinf(theta);
				DSTRFfree = cuCaddf(DSTRFfree, expikr);
				}
			DSTRFnumfree +=1;
			}
		}
	for (unsigned int i = 0; i< center_per_mol.size(); i++)
		{
		center_per_mol[i].x /= (double)mol_size[i];
		center_per_mol[i].y /= (double)mol_size[i];		
		center_per_mol[i].z /= (double)mol_size[i];
		double dx = center_per_mol[i].x - m_pos_cm[i].x;
		double dy = center_per_mol[i].y - m_pos_cm[i].y;		
		double dz = center_per_mol[i].z - m_pos_cm[i].z;
		unsigned int moltypeid = mol_type_id[i];	
		for (unsigned int j = 0; j< m_qvec.size(); j++)
			{
			vec_int qvec = m_qvec[j];
			float theta =2*M_PI*(float(qvec.x)*dx/Lx + float(qvec.y)*dy/Ly + float(qvec.z)*dz/Lz);
			cuFloatComplex expikr;
			expikr.x = cosf(theta);
			expikr.y = sinf(theta);
			DSTRFPerMolKind[moltypeid+n_kind_mol] = cuCaddf(DSTRFPerMolKind[moltypeid+n_kind_mol],expikr);
			}
		DSTRFnum[moltypeid+n_kind_mol] += 1.0;		
		}
	for (unsigned int i = 0; i< DSTRFPerMolKind.size(); i++)
		{
		DSTRFPerMolKind[i].x /= ((double)(DSTRFnum[i]*m_qvec.size()));
		}
	unsigned int NKmol = DSTRFPerMolKind.size()/2;
	for(unsigned int j =0; j< NKmol; j++)
		m_file <<m_Nf<<"   Mol"<<j<<"   "<<DSTRFPerMolKind[j+NKmol].x<<"   "<<DSTRFPerMolKind[j].x;
	if(DSTRFnumfree>0)
		m_file <<m_Nf<<"   Free particle   "<<DSTRFfree.x/double(DSTRFnumfree*m_qvec.size()); 		
	m_file <<"\n"; 
	m_Nf += 1;
	}
//--- case 17
void ConfigCheck::compute()
	{	
	std::vector<vec> pos = m_build->getPos();
	std::vector<unsigned int> type = m_build->getType();
	
	unsigned int Np = m_build->getNParticles();
	unsigned int Ntypes = m_build->getNParticleTypes();
	std::vector<vec_int> image = m_build->getImage();
	
	std::vector< std::string> bondMap = m_build->getBondTypeMap();
	std::vector<Bond> bonds = m_build->getBond();
	std::vector<Angle> angles = m_build->getAngle();
	std::vector<Dihedral> dihedrals = m_build->getDihedral();
	unsigned int nbond = bonds.size();
	unsigned int nangle = angles.size();
	unsigned int ndihedral = dihedrals.size();	
	unsigned int ntypeofbond = bondMap.size();	
//	unsigned int ndimension = m_build->getNDimensions();		
	if(image.size()==0)
		{
		image.resize(pos.size());
		}
	std::vector< std::string> type_map = m_build->getTypeMap();

	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);

    double Lx2 = Lx / double(2.0);
    double Ly2 = Ly / double(2.0);
    double Lz2 = Lz / double(2.0);
	
	double Lxinv = 0.0;
	double Lyinv = 0.0;
	double Lzinv = 0.0;
	
	if(Lx!=0.0)
		Lxinv = 1.0 / Lx;
	if(Ly!=0.0)
		Lyinv = 1.0 / Ly;
	if(Lz!=0.0)
		Lzinv = 1.0 / Lz;	

	for (unsigned int i = 0; i < Np; i++)
		{
		double pix = pos[i].x;
		double piy = pos[i].y;
		double piz = pos[i].z;		
		if(pix<-Lx2||pix>Lx2||piy<-Ly2||piy>Ly2||piz<-Lz2||piz>Lz2)
			m_file <<"Error!!! particle "<<i<<" is not in box"<<endl;	
		}

	m_mol->computeList(m_rcut);
	if(m_bondex&&nbond>0)
		m_mol->bondExclude();
	if(m_angleex&&nangle>0)
		m_mol->angleExclude();
	if(m_dihedralex&&ndihedral>0)
		m_mol->dihedralExclude();
	std::vector<unsigned int> body = m_build->getBody();		
	if(body.size()==0)
		{
		body.resize(pos.size());
		for(unsigned int i=0;i<Np;i++)
			body[i]=NO_BODY;
		}
	
	std::vector<vec_int> map = m_mol->getMap();
	std::vector<unsigned int> m_head = m_mol->getHead();
	std::vector<unsigned int> m_list = m_mol->getList();	
	vec m_width = m_mol->getWith();
	vec_uint m_dim = m_mol->getDim();	

	std::vector< vec > check_init;
	check_init.resize(Ntypes*Ntypes);
	for (unsigned int i = 0; i<Ntypes*Ntypes; i++)
		check_init[i] = vec(0.0,0.0,10000000.0);
	unsigned int space = round(double(Np)/100.0);
	if(m_dim.x<=3||m_dim.y<=3||m_dim.z<=3)
		{
		for (unsigned int i = 0; i < Np; i++)
			{
			double pix = pos[i].x + double(image[i].x)*Lx;
			double piy = pos[i].y + double(image[i].y)*Ly;
			double piz = pos[i].z + double(image[i].z)*Lz;
			unsigned int my_body = body[i];
			unsigned int typi = (unsigned int) type[i];
			for (unsigned int j = i+1; j < Np; j++)
				{
				unsigned int typj = (unsigned int) type[j];
				unsigned int pair = 0;
				if(typi <=typj)
					pair = typi*Ntypes + typj;
				else
					pair = typj*Ntypes + typi;
				double rmin = check_init[pair].z;	
				
				double pjx = pos[j].x + double(image[j].x)*Lx;
				double pjy = pos[j].y + double(image[j].y)*Ly;
				double pjz = pos[j].z + double(image[j].z)*Lz;
				unsigned int neigh_body = body[j];
				
				double dxij = pix - pjx;
				double dyij = piy - pjy;
				double dzij = piz - pjz;
				
				if(my_body != NO_BODY&&my_body == neigh_body)
					{
					if ((dxij > Lx2)||(dxij < -Lx2))
						m_file <<"Error!!! body "<<my_body<<" across the box in x direction"<<endl;
					if ((dyij > Ly2)||(dyij < -Ly2))
						m_file <<"Error!!! body "<<my_body<<" across the box in y direction"<<endl;
					if ((dzij > Lz2)||(dzij < -Lz2))				
						m_file <<"Error!!! body "<<my_body<<" across the box in z direction"<<endl;
					}
				
				dxij -= Lx * rint(dxij*Lxinv);
				dyij -= Ly * rint(dyij*Lyinv);
				dzij -= Lz * rint(dzij*Lzinv);
				
				double rsq = dxij*dxij + dyij*dyij + dzij*dzij;
				double rij = sqrt(rsq);
				
				bool bexcluded =false; 
				if (m_bodyex&& my_body != NO_BODY)
					{
					bexcluded = (my_body == neigh_body);
					}
				if(rij<rmin&&!m_mol->ifexclude(i,j)&&!bexcluded)
					{
					check_init[pair].x = double(i);
					check_init[pair].y = double(j);
					check_init[pair].z = rij;				
					}
				}
			if(space>0&&i%space==0)
				cout<<"-- check complete "<<int(i/space)<<"%"<<endl;
			}
		}
	else	
		{
		for (unsigned int idx = 0; idx< pos.size(); idx++)
			{
			unsigned int my_body = body[idx];
			double px = pos[idx].x;
			double py = pos[idx].y;
			double pz = pos[idx].z;
			
			if(Lx != 0.0&&Ly !=0.0 &&Lz!=0.0)
				{
				int shiftx = rint(px*Lxinv);
				int shifty = rint(py*Lyinv);
				int shiftz = rint(pz*Lzinv);
								
				px -=  Lx * shiftx;
				py -=  Ly * shifty;
				pz -=  Lz * shiftz;
				}	
			
			unsigned int typi = (unsigned int) type[idx];
			
			int ix = int((px+0.5*Lx)/m_width.x);
			int iy = int((py+0.5*Ly)/m_width.y);
			int iz = int((pz+0.5*Lz)/m_width.z);			
	//		unsigned int cid = cellid(ix, iy, iz, m_dim);
	//----------compute with particles in same cell
			unsigned int id = m_list[idx];
			while(id!=NO_INDEX)
				{
				double dxij = px - pos[id].x + double(image[idx].x - image[id].x)*Lx;
				double dyij = py - pos[id].y + double(image[idx].y - image[id].y)*Ly;
				double dzij = pz - pos[id].z + double(image[idx].z - image[id].z)*Lz;
				unsigned int neigh_body = body[id];
				
				if(my_body != NO_BODY&&my_body == neigh_body)
					{
					if ((dxij > Lx2)||(dxij < -Lx2))
						m_file <<"Error!!! body "<<my_body<<" across the box in x direction"<<endl;
					if ((dyij > Ly2)||(dyij < -Ly2))
						m_file <<"Error!!! body "<<my_body<<" across the box in y direction"<<endl;
					if ((dzij > Lz2)||(dzij < -Lz2))				
						m_file <<"Error!!! body "<<my_body<<" across the box in z direction"<<endl;
					}
				
				dxij -= Lx * rint(dxij*Lxinv);
				dyij -= Ly * rint(dyij*Lyinv);
				dzij -= Lz * rint(dzij*Lzinv);
				
				double rsq = dxij*dxij + dyij*dyij + dzij*dzij;
				double rij = sqrt(rsq);
		
				unsigned int typj = (unsigned int) type[id];
				unsigned int pair = 0;
				if(typi <=typj)
					pair = typi*Ntypes + typj;
				else
					pair = typj*Ntypes + typi;
				double rmin = check_init[pair].z;				

				bool bexcluded =false; 
				if (m_bodyex&& my_body != NO_BODY)
					{
					bexcluded = (my_body == neigh_body);
					}
				if(rij<rmin&&!m_mol->ifexclude(idx,id)&&!bexcluded)	
					{							
					check_init[pair].x = double(idx);
					check_init[pair].y = double(id);
					check_init[pair].z = rij;				
					}
				id = m_list[id];	
				}
	//----------compute with particles in surrrounding cells
			for(unsigned int im = 0; im <map.size(); im++)
				{
				int i = map[im].x;
				int j = map[im].y;
				int k = map[im].z;

				unsigned int jcell = cellid(ix+i, iy+j, iz+k, m_dim);
				id = m_head[jcell];
				while(id!=NO_INDEX)
					{
					double dxij = px - pos[id].x + double(image[idx].x - image[id].x)*Lx;
					double dyij = py - pos[id].y + double(image[idx].y - image[id].y)*Ly;
					double dzij = pz - pos[id].z + double(image[idx].z - image[id].z)*Lz;
					unsigned int neigh_body = body[id];
					
					if(my_body != NO_BODY&&my_body == neigh_body)
						{
						if ((dxij > Lx2)||(dxij < -Lx2))
							m_file <<"Error!!! body "<<my_body<<" across the box in x direction"<<endl;
						if ((dyij > Ly2)||(dyij < -Ly2))
							m_file <<"Error!!! body "<<my_body<<" across the box in y direction"<<endl;
						if ((dzij > Lz2)||(dzij < -Lz2))				
							m_file <<"Error!!! body "<<my_body<<" across the box in z direction"<<endl;
						}
					
					dxij -= Lx * rint(dxij*Lxinv);
					dyij -= Ly * rint(dyij*Lyinv);
					dzij -= Lz * rint(dzij*Lzinv);
				
					double rsq = dxij*dxij + dyij*dyij + dzij*dzij;
					double rij = sqrt(rsq);
			
					unsigned int typj = (unsigned int) type[id];
					unsigned int pair = 0;
					if(typi <=typj)
						pair = typi*Ntypes + typj;
					else
						pair = typj*Ntypes + typi;
					double rmin = check_init[pair].z;				

					bool bexcluded =false; 
					if (m_bodyex&& my_body != NO_BODY)
						{
						bexcluded = (my_body == neigh_body);
						}
					if(rij<rmin&&!m_mol->ifexclude(idx,id)&&!bexcluded)	
						{							
						check_init[pair].x = double(idx);
						check_init[pair].y = double(id);
						check_init[pair].z = rij;				
						}
					id = m_list[id];	
					}	
				}
			if(space>0&&idx%space==0)
				cout<<"-- check complete "<<int(idx/space)<<"%"<<endl;			
			}
		}
	std::string fname = m_build->getFilename();
	m_file <<fname<<" -- result of statistics of checking initial configure"<<endl;		
	for (unsigned int i = 0; i<Ntypes; i++)
		{
		for (unsigned int j = i; j<Ntypes; j++)
			{
			unsigned int pair = i*Ntypes + j;
			if(check_init[pair].z==10000000.0)
				m_file <<"pair "<<type_map[i]<<" "<<type_map[j]<<", r_min >= "<<m_rcut<<endl;	
			else
				m_file <<"pair "<<type_map[i]<<" "<<type_map[j]<<", r_min = "<<check_init[pair].z<<", i = "<<int(check_init[pair].x)<<", j= "<<int(check_init[pair].y)<<endl;		
			}
		}
//--------------check density distribution
	unsigned int ncell = m_dim.x*m_dim.y*m_dim.z;
	std::vector<unsigned int> density;
	density.resize(ncell);
	for(unsigned int jcell = 0; jcell <ncell; jcell++)
		{
		unsigned int id = m_head[jcell];
		while(id!=NO_INDEX)
			{
			density[jcell] +=1;
			id = m_list[id];	
			}
		}
	unsigned int max_dens =0;
	for(unsigned int jcell = 0; jcell <ncell; jcell++)
		{
		if(density[jcell]>max_dens)
			max_dens=density[jcell];
		}
	double max_density=double(max_dens)/double(m_width.x*m_width.y*m_width.z);
	double ave_density=double(Np)/(Lx*Ly*Lz);
	m_file<<"Density report: average number density "<<ave_density<<" ; local maximum number density "<<max_density<<endl;
//--------------check bond length
    std::vector<double> rmin_bond; 
    std::vector<double> rmax_bond;
	
	rmin_bond.resize(ntypeofbond);
	rmax_bond.resize(ntypeofbond);
	
    std::vector<Bond> min_bond; 
    std::vector<Bond> max_bond;	

	for(unsigned int i=0; i<ntypeofbond;i++)
		{
		rmin_bond[i] = 100000000.0;
		rmax_bond[i] = 0.0;
		min_bond.push_back(Bond("",0, 0, 0));
		max_bond.push_back(Bond("",0, 0, 0));	
		}	

	if(nbond>0)
		cout<<"-- please wait, checking bond"<<endl;			
	for(unsigned int i =0; i<nbond; i++)
		{
		unsigned int type = bonds[i].id;
		unsigned int taga = bonds[i].a;
		unsigned int tagb = bonds[i].b;
		
		double pax = pos[taga].x;
		double pay = pos[taga].y;
		double paz = pos[taga].z;
			
		double pbx = pos[tagb].x;
		double pby = pos[tagb].y;
		double pbz = pos[tagb].z;
				
		double dxij = pax - pbx;
		double dyij = pay - pby;
		double dzij = paz - pbz;
			
		if (dxij >= Lx2)
			dxij -= Lx;
		else if (dxij < -Lx2)
			dxij += Lx;
					
		if (dyij >= Ly2)
			dyij -= Ly;
		else if (dyij < -Ly2)
			dyij += Ly;
					
		if (dzij >= Lz2)
			dzij -= Lz;
		else if (dzij < -Lz2)
			dzij += Lz;			
					
		double rsq = dxij*dxij + dyij*dyij + dzij*dzij;
		double rij = sqrt(rsq);	
		if(rij<rmin_bond[type])
			{
			rmin_bond[type] = rij;
			min_bond[type] = bonds[i];
			}
		if(rij>rmax_bond[type])
			{
			rmax_bond[type] = rij;
			max_bond[type] = bonds[i];				
			}
		}
	for(unsigned int i=0; i<ntypeofbond;i++)
		{
		m_file <<"bond "<<bondMap[i]<<", r_min = "<<rmin_bond[i]<<", i = "<<min_bond[i].a<<", j= "<<min_bond[i].b<<", r_max = "<<rmax_bond[i]<<", i = "<<max_bond[i].a<<", j= "<<max_bond[i].b<<endl;
		}
	m_Nf += 1;
	}
	
//--- case 18
void RDFBetweenTypes::compute()
	{
	std::string fname = m_build->getFilename();
	string::size_type xp = fname.find("mst");
	string outs;
	if (xp!=fname.npos)
		outs = fname.replace(xp,xp+3, "type.rdf");
	else
		outs = fname+".rdf";
	ofstream fp(outs.c_str());
	
	unsigned int N = m_build->getNParticles();
	unsigned int Ntypes = m_build->getNParticleTypes();
	if(m_Nf==0)
		{
		m_rdf.resize(Ntypes*Ntypes*m_maxbin);
		m_type_map = m_build->getTypeMap();
		m_Ntype = Ntypes;
		}
	BoxSize box = m_build->getBox();
	std::vector<vec> pos = m_build->getPos();
	std::vector<unsigned int> type = m_build->getType();
	unsigned int ndimension = m_build->getNDimensions();	
	float Lx = float (box.lx);
	float Ly = float (box.ly);		 
	float Lz = float (box.lz);		 
	float Lxinv = 0.0;
	float Lyinv = 0.0;
	float Lzinv = 0.0;	
	if(Lx!=0.0)
		Lxinv = 1.0/Lx;
	if(Ly!=0.0)
		Lyinv = 1.0/Ly;
	if(Lz!=0.0)
		Lzinv = 1.0/Lz;	
	for(unsigned int typi =0; typi <Ntypes; typi++)
		{
		for(unsigned int typj =typi; typj <Ntypes; typj++)
			{
			double *r_DPD = (double*) calloc(m_maxbin,sizeof(double));	
			double *g = (double*) calloc(m_maxbin,sizeof(double));
			
			float4 *hpos;
			float4 *dpos;
			unsigned int* h_group;
			unsigned int* d_group;			
			cudaHostAlloc(&hpos, sizeof(float)*N*4, cudaHostAllocPortable);	
			cudaHostAlloc(&h_group, sizeof(unsigned int)*N, cudaHostAllocPortable);	
			unsigned int N_partial =0;
			unsigned int N_partiali =0;
			unsigned int N_partialj = 0;
			for(unsigned int i =0; i< N; i++)
				{
				if(type[i]==typi||type[i]==typj)
					{
					hpos[N_partial].x = float(pos[i].x);
					hpos[N_partial].y = float(pos[i].y);
					hpos[N_partial].z = float(pos[i].z);
					hpos[N_partial].w = float(type[i]);
					h_group[N_partial] = i;
					N_partial+=1;
					if(type[i]==typi)
						N_partiali += 1;
					if(type[i]==typj)
						N_partialj += 1;
					}
				}			
			
			int nblocks = (int)ceil((float)N_partial / (float)m_block_size);
			cudaMalloc(&dpos, sizeof(float)*N_partial*4);
			cudaMalloc(&d_group, sizeof(unsigned int)*N_partial);			
			unsigned int* scratch;
			unsigned int* d_scratch;
			unsigned int* d_gg;
			cudaHostAlloc(&scratch, sizeof(unsigned int)*nblocks*m_maxbin, cudaHostAllocPortable);	
			cudaMalloc(&d_scratch, sizeof(unsigned int)*nblocks*m_maxbin);
			cudaMemset(d_scratch, 0, sizeof(unsigned int)*nblocks*m_maxbin);		
			cudaMalloc(&d_gg, sizeof(unsigned int)*nblocks*m_maxbin*m_block_size);
			cudaMemset(d_gg, 0, sizeof(unsigned int)*nblocks*m_maxbin*m_block_size);
				

			cudaMemcpy(dpos, hpos, sizeof(float)*N_partial*4, cudaMemcpyHostToDevice);
			cudaMemcpy(d_group, h_group, sizeof(unsigned int)*N_partial, cudaMemcpyHostToDevice);
			float pi = 4.0*atan(1.0);
				   
			float rmax = 0.5*Lx;
			if (m_rmax>0.0)
				rmax = m_rmax;
			float rho  = (float)(N_partialj)*Lxinv*Lyinv*Lzinv;
			if(ndimension==2)
				rho = (float)(N_partialj)*Lxinv*Lyinv;			
			float delr = rmax/(float)m_maxbin;
			bool bytype;
			unsigned int coeff = 1;
			if(typi==typj)
				bytype = false;
			else
				{
				bytype = true;
				coeff = 2;
				}

			if (m_exclusion_mol)
				{
				d_mol_id_per_particle = m_mol->getMolIdPerParticleGPU();
				}
			if(m_exclusion_bond)
				{
				m_mol->bondExclude();
				m_exclusion_list=true;
				}
			if(m_exclusion_angle)
				{
				m_mol->angleExclude();
				m_exclusion_list=true;
				}
			if (m_exclusion_list)
				{
				d_n_exclusion = m_mol->getNExclusionGPU();
				d_exclusion_list = m_mol->getExclusionListGPU();
				}					 
				
			gpu_compute_rdf(dpos,N_partial,N,Lx,Ly,Lz,Lxinv,Lyinv,Lzinv,delr,d_gg,d_scratch,m_maxbin,d_group,
							d_n_exclusion,d_exclusion_list,d_mol_id_per_particle,m_exclusion_mol,
							m_exclusion_list,bytype,m_block_size);

			cudaMemcpy(scratch, d_scratch, sizeof(unsigned int)*nblocks*m_maxbin, cudaMemcpyDeviceToHost);

			for(unsigned int i =0; i<(unsigned int)nblocks;i++)
				{
				for(unsigned int j =0; j< m_maxbin;j++)
					g[j] += double(scratch[i*m_maxbin+j]);
				}

			double con = 4.0 * pi*rho/3.0;
			if(ndimension==2)
				con = pi*rho;
			fp<<"type "<< m_type_map[typi]<<" and type " <<m_type_map[typj]<<"\n";
			for (unsigned int bin = 0; bin < m_maxbin; bin++ )
				{ 
				double rlower = (double)(bin)*delr;
				double rupper = rlower + delr;
				r_DPD[bin] = rlower + 0.5*delr;
				double nid = con*(rupper*rupper*rupper-rlower*rlower*rlower);
				if(ndimension==2)
					nid = con*(rupper*rupper-rlower*rlower);
				g[bin] /= (double)(N_partiali*coeff)*nid;
				fp<<r_DPD[bin]<<"  "<<g[bin]<<"\n";
				m_rdf[(typi*Ntypes+typj)*m_maxbin+bin] += g[bin];
				}
			if(m_Nf==0&&typi==0&&typj==0)
				{
				for(unsigned int i=0; i<m_maxbin; i++ )
					m_r[i]=r_DPD[i];
				}
			cudaFreeHost(hpos);
			cudaFreeHost(h_group);
			cudaFree(dpos);
			cudaFree(d_group);
			
			cudaFreeHost(scratch);
			cudaFree(d_scratch);	
			cudaFree(d_gg);	
			}
		}
	fp.close();
	m_Nf += 1;	
	}

void MSTfileConversion::compute()
	{
	if(!m_lammps&&!m_gromacs&&!m_xml)
		{
        cerr << endl << "***Error! no conversion specified!" << endl << endl;
		throw runtime_error("Error MSTfileConversion::compute!");		
		}
	if(m_lammps)
		{		
		std::string fname = m_build->getFilename();
		string filetype = "lammps";
		
		if(m_build->iftrajectory())
			{
			unsigned int timestep = m_build->getTimeStep();
			ostringstream extend_fname;
			extend_fname << setfill('0') << setw(10) << timestep << "."+filetype;
			filetype = extend_fname.str();	
			}

		string::size_type xp = fname.find("mst");
		string outs;
		if (xp!=fname.npos)
			outs = fname.replace(xp,xp+3, filetype);
		else
			outs = fname+"."+filetype;
		
		ofstream fp(outs.c_str());
		unsigned int N = m_build->getNParticles();
		unsigned int Ntypes = m_build->getNParticleTypes();
		unsigned int NBtypes = m_build->getNBondTypes();
		unsigned int NAtypes = m_build->getNAngleTypes();
		unsigned int NDtypes = m_build->getNDihedralTypes();
		std::vector<Bond> bonds = m_build->getBond();		
		std::vector<Angle> angles = m_build->getAngle();
		std::vector<Dihedral> dihedrals = m_build->getDihedral();
		BoxSize box = m_build->getBox();
		std::vector<vec> pos = m_build->getPos();
		std::vector<vec_int> image = m_build->getImage();		
		std::vector<unsigned int> type = m_build->getType();
		
		double Lx = double (box.lx);
		double Ly = double (box.ly);		 
		double Lz = double (box.lz);		 
		// double Lxinv = 1.0/Lx;     
		// double Lyinv = 1.0/Ly;
		// double Lzinv = 1.0/Lz;

		fp<<"LAMMPS GALAMOST output file"<<endl;
		fp<<" "<<endl;
		fp<<N<<"  atoms"<<endl;
		fp<<bonds.size()<<"  bonds"<<endl;
		fp<<angles.size()<<"  angles"<<endl;
		fp<<dihedrals.size()<<"  dihedrals"<<endl;
		fp<<"0"<<"  impropers"<<endl;
		fp<<" "<<endl;
		
		fp<<Ntypes<<"  atom types"<<endl;
		fp<<NBtypes<<"  bond types"<<endl;
		fp<<NAtypes<<"  angle types"<<endl;
		fp<<NDtypes<<"  dihedral types"<<endl;
		fp<<"0"<<"  improper types"<<endl;
		fp<<" "<<endl;
		
		fp<<-Lx/2.0<<" "<<Lx/2.0<<"   xlo xhi "<<endl;		
		fp<<-Ly/2.0<<" "<<Ly/2.0<<"   ylo yhi "<<endl;			
		fp<<-Lz/2.0<<" "<<Lz/2.0<<"   zlo zhi "<<endl;			
		fp<<" "<<endl;		
		fp<<"Atoms"<<endl;
		fp<<" "<<endl;
		if(image.size()==0)
			{
			for (unsigned int idx = 0; idx< pos.size(); idx++)
				{
				double px = pos[idx].x;
				double py = pos[idx].y;
				double pz = pos[idx].z;
				fp<<idx+1<<" "<<type[idx]+1<<" "<<px<<" "<<py<<" "<<pz<<endl;			
				}
			}
		else
			{
			for (unsigned int idx = 0; idx< pos.size(); idx++)
				{
				vec_int imagei = image[idx];
				double px = pos[idx].x+double(imagei.x)*Lx;
				double py = pos[idx].y+double(imagei.y)*Ly;
				double pz = pos[idx].z+double(imagei.z)*Lz;
				fp<<idx+1<<" "<<type[idx]+1<<" "<<px<<" "<<py<<" "<<pz<<endl;			
				}
			}
		if(bonds.size()>0)
			{
			fp<<" "<<endl;
			fp<<"Bonds"<<endl;
			fp<<" "<<endl;
			for(unsigned int i=0; i< bonds.size();i++)
				{
				unsigned int a = bonds[i].a;
				unsigned int b = bonds[i].b;		
				unsigned int typ = bonds[i].id;
				fp<<i+1<<" "<<typ+1<<" "<<a+1<<" "<<b+1<<endl;			
				}
			}
			
		if(angles.size()>0)
			{
			fp<<" "<<endl;
			fp<<"Angles"<<endl;
			fp<<" "<<endl;
			for(unsigned int i=0; i< angles.size();i++)
				{
				unsigned int a = angles[i].a;
				unsigned int b = angles[i].b;
				unsigned int c = angles[i].c;				
				unsigned int typ = angles[i].id;
				fp<<i+1<<" "<<typ+1<<" "<<a+1<<" "<<b+1<<" "<<c+1<<endl;			
				}
			}

		if(dihedrals.size()>0)
			{
			fp<<" "<<endl;
			fp<<"Dihedrals"<<endl;
			fp<<" "<<endl;
			for(unsigned int i=0; i< dihedrals.size();i++)
				{
				unsigned int a = dihedrals[i].a;
				unsigned int b = dihedrals[i].b;
				unsigned int c = dihedrals[i].c;
				unsigned int d = dihedrals[i].d;				
				unsigned int typ = dihedrals[i].id;
				fp<<i+1<<" "<<typ+1<<" "<<a+1<<" "<<b+1<<" "<<c+1<<" "<<d+1<<endl;			
				}
			}			

		fp.close();		
		}
		
	if(m_gromacs)
		{
		std::string fname = m_build->getFilename();
		string filetype = "gro";
		
		if(m_build->iftrajectory())
			{
			unsigned int timestep = m_build->getTimeStep();
			ostringstream extend_fname;
			extend_fname << setfill('0') << setw(10) << timestep << "."+filetype;
			filetype = extend_fname.str();	
			}

		string::size_type xp = fname.find("mst");
		string outs;
		if (xp!=fname.npos)
			outs = fname.replace(xp,xp+3, filetype);
		else
			outs = fname+"."+filetype;		


		unsigned int N = m_build->getNParticles();
		std::vector< std::string > typemap = m_build->getTypeMap();
		std::vector<unsigned int > mol_id_per_particle = m_mol->getMolIdPerParticle();

		BoxSize box = m_build->getBox();
		std::vector<vec> pos = m_build->getPos();
		std::vector<vec> vel = m_build->getVel();		
		std::vector<vec_int> image = m_build->getImage();		
		std::vector<unsigned int> type = m_build->getType();
		
		double Lx = double (box.lx);
		double Ly = double (box.ly);		 
		double Lz = double (box.lz);				

		FILE *fp1;
		fp1 = fopen(outs.c_str(),"w");
		fprintf(fp1,"galaTackle convert\n");	
		fprintf(fp1,"%5d\n",N);			

		for (unsigned int k = 0; k< N; k++)
			{
			string typi = typemap[type[k]];
			stringstream s0;
			unsigned int molid = mol_id_per_particle[k];
			string mol;
			if(molid!=NO_INDEX)
				{
				s0<<molid;	
				mol = "m"+s0.str();				
				}
			else
				mol = "liq";
			
			float px = 0.0;
			float py = 0.0;
			float pz = 0.0;	
			
			if(image.size()==N)
				{
				px = pos[k].x+0.5*Lx+float(image[k].x)*Lx;
				py = pos[k].y+0.5*Ly+float(image[k].y)*Ly;
				pz = pos[k].z+0.5*Lz+float(image[k].z)*Lz;				
				}
			else
				{
				px = pos[k].x+0.5*Lx;
				py = pos[k].y+0.5*Ly;
				pz = pos[k].z+0.5*Lz;					
				}

			float vx = 0.0;
			float vy = 0.0;
			float vz = 0.0;
			if(vel.size()==N)
				{
				vx = vel[k].x;
				vy = vel[k].y;
				vz = vel[k].z;				
				}
			fprintf(fp1,"%5X%-5s%5s%5X%8.3f%8.3f%8.3f%8.4f%8.4f%8.4f\n",k+1,mol.c_str(),typi.c_str(),k+1, px, py, pz, vx, vy, vz);
			}
		fprintf(fp1,"%10.5f%10.5f%10.5f\n",Lx,Ly,Lz);	
		fclose(fp1);
		}

	if(m_xml)
		{
		std::string fname = m_build->getFilename();
		string filetype = "xml";
		
		if(m_build->iftrajectory())
			{
			unsigned int timestep = m_build->getTimeStep();
			ostringstream extend_fname;
			extend_fname << setfill('0') << setw(10) << timestep << "."+filetype;
			filetype = extend_fname.str();	
			}

		string::size_type xp = fname.find("mst");
		string outs;
		if (xp!=fname.npos)
			outs = fname.replace(xp,xp+3, filetype);
		else
			outs = fname+"."+filetype;
		
		unsigned int Np = m_build->getNParticles();
		std::vector< std::string > typemap = m_build->getTypeMap();
		
		BoxSize box = m_build->getBox();
		std::vector<vec> pos = m_build->getPos();
		std::vector<vec_int> image = m_build->getImage();		
		std::vector<unsigned int> type = m_build->getType();
		
		std::vector<Bond> bonds = m_build->getBond();
		std::vector<Angle> angles = m_build->getAngle();
		std::vector<Dihedral> dihedrals = m_build->getDihedral();
		
		double Lx = double (box.lx);
		double Ly = double (box.ly);		 
		double Lz = double (box.lz);
		

		ofstream f(outs.c_str());
		f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << "\n";
		f << "<galamost_xml version=\"1.3\">" << "\n";
		f << "<configuration time_step=\"" << m_build->getTimeStep() << "\" "
		<< "dimensions=\"" << m_build->getNDimensions() << "\" "
		<< "natoms=\"" << Np << "\" "
		<< ">" << "\n";
		f << "<box " << "lx=\""<< Lx << "\" ly=\""<< Ly << "\" lz=\""<< Lz << "\"/>" << "\n";
		
		
		f << "<position num=\"" << Np << "\">" << "\n";
		
		for (unsigned int i = 0; i < Np; i++)
			{
			double x = pos[i].x;
			double y = pos[i].y;
			double z = pos[i].z;
		
			f <<setiosflags(ios::fixed)<<setprecision(m_nprecision)<<setw(m_nprecision+m_nhead)<< x <<setw(m_nprecision+m_nhead)<< y <<setw(m_nprecision+m_nhead)<< z << "\n";
			
			if (!f.good())
				{
				cerr << endl << "***Error! Unexpected error writing position" << endl << endl;
				throw runtime_error("Error writing dump file");
				}
			}			
		
		f <<"</position>" << "\n";
		
		f <<"<type num=\"" << Np << "\">" << "\n";
		
		for (unsigned int i = 0; i < Np; i++)
			{
			f << typemap[type[i]] << "\n";
			}
			
		f <<"</type>" << "\n";
		
		if (image.size()==Np)
			{
			f << "<image num=\"" << Np << "\">" << "\n";
		
			for (unsigned int i = 0; i < Np; i++)
				{
				int x = image[i].x;
				int y = image[i].y;
				int z = image[i].z;
				
				f << x << " " << y << " "<< z << "\n";
				
				if (!f.good())
					{
					cerr << endl << "***Error! Unexpected error writing image" << endl << endl;
					throw runtime_error("Error writing dump file");
					}
				}
		
			f <<"</image>" << "\n";
			}
			
		unsigned int nbond = bonds.size();
		if(nbond>0)
			{
			f << "<bond num=\"" << nbond << "\">" << "\n";
			
			for (unsigned int i = 0; i < bonds.size(); i++)
				{
				Bond bond = bonds[i];
				f << bond.type << " " << bond.a << " " << bond.b << "\n";
				}					
			
			if (!f.good())
					{
					cerr << endl << "***Error! Unexpected error writing bond" << endl << endl;
					throw runtime_error("Error writing dump file");
					}
			f << "</bond>" << "\n";			
			}
			
		unsigned int nangle = angles.size();
		if(nangle>0)
			{
			f << "<angle num=\"" << nangle << "\">" << "\n";
			
			for (unsigned int i = 0; i < angles.size(); i++)
				{
				Angle angle = angles[i];
				f << angle.type << " " << angle.a << " " << angle.b << " " << angle.c << "\n";
				}					
			
			if (!f.good())
					{
					cerr << endl << "***Error! Unexpected error writing angle" << endl << endl;
					throw runtime_error("Error writing dump file");
					}
			f << "</angle>" << "\n";			
			}

		unsigned int ndihedral = dihedrals.size();
		if(ndihedral>0)
			{
			f << "<dihedral num=\"" << ndihedral << "\">" << "\n";
			
			for (unsigned int i = 0; i < dihedrals.size(); i++)
				{
				Dihedral dihedral = dihedrals[i];
				f << dihedral.type << " " << dihedral.a << " " << dihedral.b << " " << dihedral.c << " " << dihedral.d <<"\n";
				}					
			
			if (!f.good())
					{
					cerr << endl << "***Error! Unexpected error writing dihedral" << endl << endl;
					throw runtime_error("Error writing dump file");
					}
			f << "</dihedral>" << "\n";			
			}			
			
	
		
		f << "</configuration>" << "\n";
		f << "</galamost_xml>" << "\n";
		}		
		
	m_Nf += 1;	
	}	
	
unsigned int switchNameToIndex(std::vector< std::string >& typemap, std::string name)
	{
    for (unsigned int i = 0; i < typemap.size(); i++)
        {
        if (typemap[i] == name)
            return i;
        }

	cerr << endl << "Unable to switch name "<<name<< " to index, failed!" << endl << endl;
	throw runtime_error("Error switchNameToIndex");
	}
	
bool existType(std::vector< std::string >& typemap, std::string name)
	{
    for (unsigned int i = 0; i < typemap.size(); i++)
        {
        if (typemap[i] == name)
            return true;
        }
	typemap.push_back(name);
	return false;
	}	
	
std::string etrim(std::string s)
	{
	unsigned int b=0;
	unsigned int e=0;
	for(unsigned int i=0;i<s.size();i++)
	{
		if(s[i]=='<')
		b = i;
		else if(s[i]=='>')
		e = i;
	}
	if(e>b)
		s=s.substr(b,e-b+1);
	return s;
	}	
	
void exyzFromQuaternion(vec4 &quat, vec4 &ex_space, vec4 &ey_space, vec4 &ez_space)
    {
    // ex_space
    ex_space.x = quat.x * quat.x + quat.y * quat.y - quat.z * quat.z - quat.w * quat.w;
    ex_space.y = double(2.0) * (quat.y * quat.z + quat.x * quat.w);
    ex_space.z = double(2.0) * (quat.y * quat.w - quat.x * quat.z);
    
    // ey_space
    ey_space.x = double(2.0) * (quat.y * quat.z - quat.x * quat.w);
    ey_space.y = quat.x * quat.x - quat.y * quat.y + quat.z * quat.z - quat.w * quat.w;
    ey_space.z = double(2.0) * (quat.z * quat.w + quat.x * quat.y);
    
    // ez_space
    ez_space.x = double(2.0) * (quat.y * quat.w + quat.x * quat.z);
    ez_space.y = double(2.0) * (quat.z * quat.w - quat.x * quat.y);
    ez_space.z = quat.x * quat.x - quat.y * quat.y - quat.z * quat.z + quat.w * quat.w;
    }

void PatchToParticle::compute()
	{
	std::string fname = m_build->getFilename();
	unsigned int Ntypes = m_build->getNParticleTypes();		
	std::vector< std::string > typemap =  m_build->getTypeMap();
	unsigned int N = m_build->getNParticles();
	
	bool output_image = false;
	bool output_shape = false;
	std::vector< vec > PatchPos;
	std::vector< std::string > PatchType; 	
	
	std::vector< vec_int > NPatch; 
	std::vector< vec > sigma; 	
	std::vector< vec > epsilon; 	
	NPatch.resize(Ntypes);
	sigma.resize(Ntypes); 	
	epsilon.resize(Ntypes);	
	unsigned int npa = 0;
	
    ifstream file;
    file.open(fname.c_str());	

	if (!file.good())
		{
		cerr << endl << "Unable to open file " << fname.c_str() << endl << endl;
		throw runtime_error("Error PatchToParticle::compute()");
		}
		
	file.seekg(0,ios::beg);			
	std::string line;		
	std::string aim1 = "<Patches>";
	std::string aim2 = "</Patches>";
	while(getline(file, line) && etrim(line) != aim1)
		{
		}
	if (!file.eof())
		{	
		cout << "read: " << line << endl;
		while(getline(file, line) && etrim(line) != aim2)
			{
			istringstream parser(line);
			unsigned int i1;
			if(parser.good())
				{
				std::string s0;
				while(parser >>s0>>i1)
					{
					unsigned int typ = switchNameToIndex(typemap, s0);
					NPatch[typ] = vec_int(i1, npa, 0);
					npa += i1;
					}
				}
			else
				{
				cerr << endl << "Unable to parse line of patche Nber, parser.good() failed" << endl << endl;
				throw runtime_error("PatchToParticle::compute()");			
				}
				
			for(unsigned int i=0; i< i1; i++)
				{
				if(getline(file, line) && etrim(line) != aim2)
					{
					istringstream parser2(line);
					if(parser2.good())
						{
						std::string s0;						
						double c1;
						double c2;
						double c3;
						double c4;
						while(parser2 >>s0>>c1>>c2>>c3>>c4)
							{
							double len = sqrt(c2*c2 + c3*c3 + c4*c4);
							PatchPos.push_back(vec(c2/len, c3/len, c4/len));
							PatchType.push_back(s0);							
							}
						}
					else
						{
						cerr << endl << "Unable to parse line of patches, parser2.good() failed" << endl << endl;
						throw runtime_error("PatchToParticle::compute()");			
						}
					}
				}
			}
		}
		
	file.clear();	
	file.seekg(0,ios::beg);			
	aim1 = "<Aspheres>";
	aim2 = "</Aspheres>";
	while(getline(file, line) && etrim(line) != aim1)
		{
		}
	if (!file.eof())
		{	
		cout << "read " << etrim(line) << endl;	
		while(getline(file, line) && etrim(line) != aim2)
			{
			istringstream parser(line);
			if(parser.good())
				{
				output_shape = true;
				std::string s0;
				double a, b, c, eia_one, eib_one, eic_one;
				while(parser>>s0>>a>>b>>c>>eia_one>>eib_one>>eic_one)
					{
					unsigned int typ = switchNameToIndex(typemap, s0);
					sigma[typ] = vec(a, b, c);
					epsilon[typ] = vec(eia_one, eib_one, eic_one);
					}
				}
			else
				{
				cerr << endl << "Unable to parse line of ashperes, parser.good() failed" << endl << endl;
				throw runtime_error("PatchToParticle::compute()");			
				}
			}
		}
		
	std::map<std::string, vec4> spot;
	file.clear();	
	file.seekg(0, ios::beg);		
	aim1 = "<Spots>";
	aim2 = "</Spots>";
	while(getline(file, line) && etrim(line) != aim1)
		{
		}		
	if (!file.eof())
		{
		cout <<"read " << etrim(line) << endl;
//		cout<<"	"<<"spot type"<<", "<<"sx"<<", "<<"sy" <<", "<<"sz"<< endl;			

		while(getline(file, line) && etrim(line) != aim2)
			{
			istringstream parser(line);
			if(parser.good())
				{
				std::string n0;
				double s0;
				double s1;
				double s2;
				while(parser >>n0>>s0>>s1>>s2)
					{
					double rsq = s0*s0 + s1*s1 + s2*s2;
					spot.insert(pair<std::string, vec4>(n0, vec4(s0, s1, s2, rsq)));
//					cout<<"	"<<n0<<", "<<s0<<", "<<s1 <<", "<<s2<< endl;			
					}
				}
			else
				{
				cerr << endl << "Unable to parse line, parser.good() failed" << endl << endl;
				throw runtime_error("Error parser(line)");			
				}
			}
		}

		
	std::map<std::string, Str2> bond_ellip;
	file.clear();	
	file.seekg(0, ios::beg);		
	aim1 = "<BondEllipsoid>";
	aim2 = "</BondEllipsoid>";
	while(getline(file, line) && etrim(line) != aim1)
		{
		}		
	if (!file.eof())
		{
		cout <<"read " << etrim(line) << endl;
//		cout<<"	"<<"bond type"<<", "<<"spot type"<<", "<<"spot type"<< endl;			

		while(getline(file, line) && etrim(line) != aim2)
			{
			istringstream parser(line);
			if(parser.good())
				{
				std::string s0;
				std::string s1;
				std::string s2;
				while(parser >>s0>>s1>>s2)
					{
					bond_ellip.insert(pair<std::string, Str2>(s0, Str2(s1, s2)));
//					cout<<"	"<<s0<<", "<<s1<<", "<<s2 << endl;			
					}
				}
			else
				{
				cerr << endl << "Unable to parse line, parser.good() failed" << endl << endl;
				throw runtime_error("Error parser(line)");			
				}
			}
		}

		
	std::vector<vec> pos = m_build->getPos();
	std::vector<vec4> quat = m_build->getQuaternion();
	std::vector<unsigned int> type = m_build->getType();
	std::vector<vec_int> image = m_build->getImage();
	if(image.size()==N)
		output_image=true;
	if(quat.size()!=N)
		{
		cerr << endl << "***Error! No quaternion!" << endl << endl;
		throw runtime_error("PatchToParticle::compute()");		
		}

	std::vector<std::string> newtype;
	std::vector<vec> newpos;
	std::vector<vec4> newquaternion;
	std::vector<vec_int> newimage;	
	std::vector<int> newsequence;
	newsequence.resize(N);
	
	unsigned int idx=0;
	for (unsigned int i = 0; i< N; i++)
		{
		vec posi = pos[i];				
		vec4 q4 = quat[i];
		unsigned int typ = type[i];
		string typname = typemap[typ];
		newsequence[i]= -1;
		
		double ori = q4.x*q4.x + q4.y*q4.y + q4.z*q4.z + q4.w*q4.w;
		if(!m_filter_sphere||ori>0.1)
			{
			newsequence[i]=int(idx);
			newpos.push_back(posi);
			newtype.push_back(typname);
			newquaternion.push_back(q4);
			if(output_image)
				newimage.push_back(image[i]);
			idx+=1;			
			}

		if(ori>0.1)
			{
			vec4 ar0, ar1, ar2;
			exyzFromQuaternion(q4, ar0, ar1, ar2);
			vec_int npai = NPatch[typ];
	
			for(unsigned int j=0; j< (unsigned int)npai.x; j++)
				{
				vec pi1 = PatchPos[npai.y+j];		
				vec ri, posp;
				ri.x = ar0.x * pi1.x + ar1.x * pi1.y + ar2.x * pi1.z;
				ri.y = ar0.y * pi1.x + ar1.y * pi1.y + ar2.y * pi1.z;
				ri.z = ar0.z * pi1.x + ar1.z * pi1.y + ar2.z * pi1.z;
				posp.x = posi.x + m_separatedis*ri.x;
				posp.y = posi.y + m_separatedis*ri.y;
				posp.z = posi.z + m_separatedis*ri.z;
	
				newpos.push_back(posp);
				newtype.push_back(PatchType[npai.y+j]);
				newquaternion.push_back(q4);
				if(output_image)
					newimage.push_back(image[i]);
				idx+=1;
				}
			}
		}


	std::vector<Bond> bonds = m_build->getBond();
	bool vir_site=false;
	if(bond_ellip.size()>0&&spot.size()>0)
		{
		for (unsigned int i = 0; i < bonds.size(); i++)
			{
			Bond bond = bonds[i];
			Str2 be = bond_ellip[bond.type];
			vec4 spota = spot[be.x];
			vec4 spotb = spot[be.y];
			if(spota.w>0)
				{
				newsequence.push_back(int(idx));
				vec posi = pos[bond.a];				
				vec4 q4 = quat[bond.a];
				vec4 ar0, ar1, ar2;
				exyzFromQuaternion(q4, ar0, ar1, ar2);
				vec ri, posp;
				ri.x = ar0.x * spota.x + ar1.x * spota.y + ar2.x * spota.z;
				ri.y = ar0.y * spota.x + ar1.y * spota.y + ar2.y * spota.z;
				ri.z = ar0.z * spota.x + ar1.z * spota.y + ar2.z * spota.z;
				posp.x = posi.x + ri.x;
				posp.y = posi.y + ri.y;
				posp.z = posi.z + ri.z;	
	
				newpos.push_back(posp);
				newtype.push_back("vir");
				vir_site=true;
				newquaternion.push_back(vec4(0.0, 0.0, 0.0, 0.0));
				if(output_image)
					newimage.push_back(image[bond.a]);	
				bonds[i].a=newsequence.size()-1;
				idx+=1;				
				}
				
			if(spotb.w>0)
				{
				newsequence.push_back(int(idx));
				vec posi = pos[bond.b];				
				vec4 q4 = quat[bond.b];
				vec4 ar0, ar1, ar2;
				exyzFromQuaternion(q4, ar0, ar1, ar2);
				vec ri, posp;
				ri.x = ar0.x * spotb.x + ar1.x * spotb.y + ar2.x * spotb.z;
				ri.y = ar0.y * spotb.x + ar1.y * spotb.y + ar2.y * spotb.z;
				ri.z = ar0.z * spotb.x + ar1.z * spotb.y + ar2.z * spotb.z;
				posp.x = posi.x + ri.x;
				posp.y = posi.y + ri.y;
				posp.z = posi.z + ri.z;	
	
				newpos.push_back(posp);
				newtype.push_back("vir");
				vir_site=true;
				newquaternion.push_back(vec4(0.0, 0.0, 0.0, 0.0));
				if(output_image)
					newimage.push_back(image[bond.b]);
				bonds[i].b=newsequence.size()-1;				
				idx+=1;				
				}
			}		
		}


	std::string fname_out = fname;	
	std::string::size_type xp = fname_out.find("xml");
	std::string outs;
	if (xp!=fname_out.npos)
		outs = fname_out.replace(xp,xp+3, "new.xml");
	else
		outs = fname_out+".new.xml";
	
	ofstream f(outs.c_str());

	BoxSize box = m_build->getBox();
	double Lx = double (box.lx);
	double Ly = double (box.ly);		 
	double Lz = double (box.lz);		
	unsigned int Np = newpos.size();
	
	
    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << "\n";
    f << "<galamost_xml version=\"1.3\">" << "\n";
    f << "<configuration time_step=\"" << m_build->getTimeStep() << "\" "
      << "dimensions=\"" << m_build->getNDimensions() << "\" "
      << "natoms=\"" << Np << "\" "
      << ">" << "\n";
    f << "<box " << "lx=\""<< Lx << "\" ly=\""<< Ly << "\" lz=\""<< Lz << "\"/>" << "\n";
 

	f << "<position num=\"" << Np << "\">" << "\n";

	for (unsigned int i = 0; i < Np; i++)
		{
		double x = newpos[i].x;
		double y = newpos[i].y;
		double z = newpos[i].z;
	
		f <<setiosflags(ios::fixed)<<setprecision(m_nprecision)<<setw(m_nprecision+m_nhead)<< x <<setw(m_nprecision+m_nhead)<< y <<setw(m_nprecision+m_nhead)<< z << "\n";
		
		if (!f.good())
			{
			cerr << endl << "***Error! Unexpected error writing position" << endl << endl;
			throw runtime_error("Error writing dump file");
			}
		}			
	
	f <<"</position>" << "\n";

	f <<"<type num=\"" << Np << "\">" << "\n";
	
	for (unsigned int i = 0; i < Np; i++)
		{
		f << newtype[i] << "\n";
		}
		
	f <<"</type>" << "\n";

    if (output_image)
        {
        f << "<image num=\"" << Np << "\">" << "\n";
	
		for (unsigned int i = 0; i < Np; i++)
			{
			int x = newimage[i].x;
			int y = newimage[i].y;
			int z = newimage[i].z;
			
			f << x << " " << y << " "<< z << "\n";
			
			if (!f.good())
				{
				cerr << endl << "***Error! Unexpected error writing image" << endl << endl;
				throw runtime_error("Error writing dump file");
				}
			}

        f <<"</image>" << "\n";
        }

	f <<"<quaternion num=\"" << Np << "\">" << "\n";
	for (unsigned int i = 0; i < Np; i++)
		{
		vec4 s = newquaternion[i];
		f << s.x << " " << s.y << " " << s.z << " " << s.w << "\n";
		}		
	
	if (!f.good())
		{
		cerr << endl << "***Error! Unexpected error writing quaternion" << endl << endl;
		throw runtime_error("Error writing dump file");
		}
	f <<"</quaternion>" << "\n";


	unsigned int nbond = bonds.size();
	if(nbond>0)
		{
		unsigned int newnbond =0;
		for (unsigned int i = 0; i < bonds.size(); i++)
			{
			Bond bond = bonds[i];
			if(newsequence[bond.a]>=0&&newsequence[bond.b]>=0)
				newnbond+=1;
			}		
		if(newnbond>0)
			{
			f << "<bond num=\"" << nbond << "\">" << "\n";
		
			for (unsigned int i = 0; i < bonds.size(); i++)
				{
				Bond bond = bonds[i];
				if(newsequence[bond.a]>=0&&newsequence[bond.b]>=0)
					f << bond.type << " " << newsequence[bond.a]  << " " << newsequence[bond.b] << "\n";
				}					
		
			if (!f.good())
					{
					cerr << endl << "***Error! Unexpected error writing bond" << endl << endl;
					throw runtime_error("Error writing dump file");
					}
			f << "</bond>" << "\n";			
			}
		}
	std::vector< std::string > output_patchtype; 
	if (output_shape)
        {
		f << "<Aspheres>" << "\n";		
		for (unsigned int i=0; i< typemap.size(); i++)
			{
			f<<typemap[i]<<" "<<sigma[i].x<<" "<<sigma[i].y<<" "<<sigma[i].z<<" "<<epsilon[i].x<<" "<<epsilon[i].y<<" "<<epsilon[i].z<<endl;

			vec_int npai = NPatch[i];
			for(unsigned int j=0; j< (unsigned int)npai.x; j++)
				{
				std::string ptype = PatchType[npai.y+j];
				if(!existType(output_patchtype, ptype))
					f<<ptype<<" "<<sigma[i].x*m_scale<<" "<<sigma[i].y*m_scale<<" "<<sigma[i].z*m_scale<<" "<<epsilon[i].x<<" "<<epsilon[i].y<<" "<<epsilon[i].z<<endl;					
				}	
			}
		if(vir_site)
			f<<"vir"<<" "<<0.0001<<" "<<0.0001<<" "<<0.0001<<" "<<1.0<<" "<<1.0<<" "<<1.0<<endl;				
        f <<"</Aspheres>" << "\n";
        }		

    f << "</configuration>" << "\n";
    f << "</galamost_xml>" << "\n";		

	f.close();		

	m_Nf += 1;	
	}	

	
	