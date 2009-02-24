/**
 * 
 */
package edu.uoregon.tau.perfexplorer.glue;

import java.util.List;


import edu.uoregon.tau.perfdmf.Trial;
import edu.uoregon.tau.perfexplorer.clustering.AnalysisFactory;

/**
 * @author khuck
 *
 */
public class CopyOperation extends AbstractPerformanceOperation {

	/**
	 * @param input
	 */
	public CopyOperation(PerformanceResult input) {
		super(input);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param trial
	 */
	public CopyOperation(Trial trial) {
		super(trial);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param inputs
	 */
	public CopyOperation(List<PerformanceResult> inputs) {
		super(inputs);
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see glue.PerformanceAnalysisOperation#processData()
	 */
	public List<PerformanceResult> processData() {
		for (PerformanceResult input : inputs) {
			PerformanceResult output = new DefaultResult(input);
			this.outputs.add(output);
		}
		return outputs;
	}

}
