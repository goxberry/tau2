package edu.uoregon.tau.perfexplorer.client;

import edu.uoregon.tau.perfexplorer.common.EngineType;
import edu.uoregon.tau.perfexplorer.common.PerfExplorerOutput;

public class PerfExplorerNoGUI {
	private static String tauHome;
	private static String tauArch;
	private final PerfExplorerConnection connection;
	private final PerfExplorerModel model;

	public PerfExplorerNoGUI (String configFile,
	EngineType analysisEngine, boolean quiet) {
		PerfExplorerOutput.setQuiet(quiet);
		PerfExplorerConnection.setStandalone(true);
		PerfExplorerConnection.setConfigFile(configFile);
		PerfExplorerConnection.setAnalysisEngine(analysisEngine);
		connection = PerfExplorerConnection.getConnection();
		model = PerfExplorerModel.getModel();
	}
}
