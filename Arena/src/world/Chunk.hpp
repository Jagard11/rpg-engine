// Check if this chunk has a mesh already
bool hasMesh() const { return !m_meshVertices.empty(); }

// Check if the chunk is exposed at all
bool isExposed() const { return m_exposureMask.isExposed(); } 