/**
 * 
 */
package edu.uoregon.tau.perfexplorer.glue;

import java.util.List;


import edu.uoregon.tau.perfdmf.Trial;
import edu.uoregon.tau.perfexplorer.clustering.RawDataInterface;

/**
 * @author khuck
 *
 */
public class SupportVectorOperation extends NaiveBayesOperation {

    protected final String trainString = "Support Vector Training";
    protected final String testString = "Support Vector Test";

	/**
	 * @param input
	 * @param metric
	 * @param type
	 */
	public SupportVectorOperation(PerformanceResult input, String metric,
			int type) {
		super(input, metric, type);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param trial
	 */
	public SupportVectorOperation(Trial trial) {
		super(trial);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param inputs
	 */
	public SupportVectorOperation(List<PerformanceResult> inputs) {
		super(inputs);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param data
	 */
	protected void getClassifierFromFactory(RawDataInterface data) {
		this.classifier = factory.createSupportVectorClassifier(data);
	}


}
