
#include <map>
#include <cstdlib>
#include <mpi.h>

using namespace std;

typedef std::map<int,int> rank_map;
typedef std::map<MPI_Comm,rank_map> comm_map;
// this is STATIC, because otherwise it can get freed by std::map more than once,
// when using tau_exec on an instrumented program.
static comm_map comms;


extern "C"
int TauTranslateRankToWorld(MPI_Comm comm, int rank)
{
    if (comm != MPI_COMM_WORLD) {
        // If the rank_map doesn't exist, it is created
        rank_map & comm_ranks = comms[comm];

        rank_map::iterator it = comm_ranks.find(rank);
        if (it != comm_ranks.end()) {
            return it->second;
        } else {
            int result;
            int worldrank;

            PMPI_Comm_compare(comm, MPI_COMM_WORLD, &result);
            if(result == MPI_CONGRUENT || result == MPI_IDENT) {
                worldrank = rank;
            } else {
                MPI_Group commGroup;
                MPI_Group worldGroup;
                int ranks[1];
                int worldranks[1];

                ranks[0] = rank;
                PMPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
                PMPI_Comm_group(comm, &commGroup);
                PMPI_Group_translate_ranks(commGroup, 1, ranks, worldGroup, worldranks);

                worldrank = worldranks[0];
            }
            comm_ranks[rank] = worldrank;
            return worldrank;
        }
    }
    return rank;
}

