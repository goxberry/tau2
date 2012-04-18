package edu.uoregon.tau.perfdmf;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.*;

import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.helpers.XMLReaderFactory;

import edu.uoregon.tau.common.Gzip;
import edu.uoregon.tau.perfdmf.database.DB;

/**
 * Reads a single trial from the database
 */
public class TAUdbDataSource extends DataSource {

    private DatabaseAPI databaseAPI;
    //private volatile boolean abort = false;
    //private volatile int totalItems = 0;
    //private volatile int itemsDone = 0;

    private class XMLParser extends DefaultHandler {
        private StringBuffer accumulator = new StringBuffer();
        private String currentName = "";
        private Thread currentThread;

        public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
            accumulator = new StringBuffer();
            if (localName.equals("CommonProfileAttributes")) {
                currentThread = null;
            } else if (localName.equals("ProfileAttributes")) {
                int node = Integer.parseInt(attributes.getValue("node"));
                int context = Integer.parseInt(attributes.getValue("context"));
                int threadID = Integer.parseInt(attributes.getValue("thread"));
                currentThread = getThread(node, context, threadID);
            }
        }

        public void endElement(String uri, String localName, String qName) throws SAXException {
            if (localName.equals("name")) {
                currentName = accumulator.toString().trim();
            } else if (localName.equals("value")) {
                String currentValue = accumulator.toString().trim();
                if (currentThread == null) {
                    getMetaData().put(currentName, currentValue);
                } else {
                    currentThread.getMetaData().put(currentName, currentValue);
                    getUncommonMetaData().put(currentName, currentValue);
                }
            }
        }

        public void characters(char[] ch, int start, int length) throws SAXException {
            accumulator.append(ch, start, length);
        }
    }

    public TAUdbDataSource(DatabaseAPI dbAPI) {
        super();
        this.setMetrics(new Vector<Metric>());
        this.databaseAPI = dbAPI;
    }

    public int getProgress() {
        return 0;
        //return DatabaseAPI.getProgress();
    }

    public void cancelLoad() {
        //abort = true;
        return;
    }

    private void fastGetIntervalEventData(int trialID, Map<Integer, Function> ieMap, Map<Integer, Metric> metricMap) throws SQLException {
        int numMetrics = getNumberOfMetrics();
        DB db = databaseAPI.getDb();

 
		StringBuffer buff = new StringBuffer();
		// Joins timer_callpath to the rest to get calls and subroutines
		buff.append(" SELECT  sub.timer, metric, node, context, sub.thread,  inclusive, exclusive, calls, subroutines FROM (");

		// Joins NCT to the timer values
		buff.append("SELECT thread.node_rank as node , thread.context_rank as context, thread.thread_rank as thread, ");
		buff.append("values.metric as metric, values.inclusive as inclusive, values.exclusive as exclusive, values.timer as timer FROM ");
		buff.append(db.getSchemaPrefix() + "thread as thread JOIN (");

		// Gets the values for the given trial id
		buff.append("SELECT v.timer as timer, v.thread as thread, v.metric as metric , v.inclusive_value as inclusive, v.exclusive_value as exclusive FROM ");
		buff.append(db.getSchemaPrefix() + "timer t JOIN ");
		buff.append(db.getSchemaPrefix()
				+ "timer_value v  ON t.id=v.timer WHERE t.trial=" + trialID);

		// Close the NCT join
		buff.append(") as values ON thread.id=values.thread");

		// Close the calls/subroutine join
		buff.append(") as sub JOIN "
				+ db.getSchemaPrefix()
				+ "timer_callpath as timer_callpath ON timer_callpath.timer=sub.timer");

        /*
         1 - timer
         2 - metric
         3 - node
         4 - context
         5 - thread
         6 - inclusive
         7 - exclusive
         8 - num_calls
         9 - num_subrs
         */

        // get the results
        long time = System.currentTimeMillis();
        System.out.println(buff.toString());
        ResultSet resultSet = db.executeQuery(buff.toString());
        time = (System.currentTimeMillis()) - time;
        //System.out.println("Query : " + time);
        //System.out.print(time + ", ");

        time = System.currentTimeMillis();
        while (resultSet.next() != false) {
//SELECT node, context, thread, metric, incl, exc, calls, sub FROM timer_values v, JOIN timer t ON v.timer=t.id
            int intervalEventID = resultSet.getInt(1);
            Function function = ieMap.get(new Integer(intervalEventID));

            int nodeID = resultSet.getInt(3);
            int contextID = resultSet.getInt(4);
            int threadID = resultSet.getInt(5);

            Thread thread = addThread(nodeID, contextID, threadID);
            FunctionProfile functionProfile = thread.getFunctionProfile(function);

            if (functionProfile == null) {
                functionProfile = new FunctionProfile(function, numMetrics);
                thread.addFunctionProfile(functionProfile);
            }

            int metricIndex = metricMap.get(new Integer(resultSet.getInt(2))).getID();
            double inclusive, exclusive;

            inclusive = resultSet.getDouble(6);
            exclusive = resultSet.getDouble(7);
            double numcalls = resultSet.getDouble(8);
            double numsubr = resultSet.getDouble(9);

            functionProfile.setNumCalls(numcalls);
            functionProfile.setNumSubr(numsubr);
            functionProfile.setExclusive(metricIndex, exclusive);
            functionProfile.setInclusive(metricIndex, inclusive);
        }
        time = (System.currentTimeMillis()) - time;
        //System.out.println("Processing : " + time);
        //System.out.print(time + ", ");

        resultSet.close();
    }

    private void downloadMetaData() {
        try {
            DB db = databaseAPI.getDb();
            StringBuffer joe = new StringBuffer();
            joe.append(" SELECT " + Trial.XML_METADATA_GZ);
            joe.append(" FROM TRIAL WHERE id = ");
            joe.append(databaseAPI.getTrial().getID());
            ResultSet resultSet = db.executeQuery(joe.toString());
            resultSet.next();
            InputStream compressedStream = resultSet.getBinaryStream(1);
            String metaDataString = Gzip.decompress(compressedStream);
            if (metaDataString != null) {
                XMLReader xmlreader = XMLReaderFactory.createXMLReader("org.apache.xerces.parsers.SAXParser");
                XMLParser parser = new XMLParser();
                xmlreader.setContentHandler(parser);
                xmlreader.setErrorHandler(parser);
                ByteArrayInputStream input = new ByteArrayInputStream(metaDataString.getBytes());
                xmlreader.parse(new InputSource(input));
            }
        } catch (IOException e) {
            // oh well, no metadata
        } catch (SAXException e) {
            // oh well, no metadata
        } catch (SQLException e) {
            // oh well, no metadata
        }
    }

    public void load() throws SQLException {

        //System.out.println("Processing data, please wait ......");
        long time = System.currentTimeMillis();
        int trialID = databaseAPI.getTrial().getID();
        DB db = databaseAPI.getDb();
        StringBuffer joe = new StringBuffer();
        joe.append("SELECT id, name ");
        joe.append("FROM " + db.getSchemaPrefix() + "metric ");
        joe.append("WHERE trial = ");
        joe.append(databaseAPI.getTrial().getID());
        joe.append(" ORDER BY id ");

        Map<Integer, Metric> metricMap = new HashMap<Integer, Metric>();

        ResultSet resultSet = db.executeQuery(joe.toString());
        int numberOfMetrics = 0;
        while (resultSet.next() != false) {
            int id = resultSet.getInt(1);
            String name = resultSet.getString(2);
            Metric metric = this.addMetricNoCheck(name);
            metric.setDbMetricID(id);
            metricMap.put(new Integer(id), metric);
            numberOfMetrics++;
        }
        resultSet.close();

        // map Interval Event ID's to Function objects
        Map<Integer, Function> ieMap = new HashMap<Integer, Function>();

        // iterate over interval events (functions), create the function objects and add them to the map
        List<IntervalEvent> intervalEvents = databaseAPI.getIntervalEvents();
        ListIterator<IntervalEvent> lIE = intervalEvents.listIterator();
        while (lIE.hasNext()) {
            IntervalEvent ie = lIE.next();
            Function function = this.addFunction(ie.getName(), numberOfMetrics);
            addGroups(ie.getGroup(), function);
            ieMap.put(new Integer(ie.getID()), function);
        }

        //getIntervalEventData(ieMap);
        fastGetIntervalEventData(trialID,ieMap, metricMap);

        // map Interval Event ID's to Function objects
        Map<Integer, UserEvent> aeMap = new HashMap<Integer, UserEvent>();
System.err.println("Warning not loading Atomic Events from TAUdb yet");
//        ListIterator<AtomicEvent> lAE = databaseAPI.getAtomicEvents().listIterator();
//        while (lAE.hasNext()) {
//            AtomicEvent atomicEvent = lAE.next();
//            UserEvent userEvent = addUserEvent(atomicEvent.getName());
//            aeMap.put(new Integer(atomicEvent.getID()), userEvent);
//        }

//        ListIterator<AtomicLocationProfile> lAD = databaseAPI.getAtomicEventData().listIterator();
//        while (lAD.hasNext()) {
//            AtomicLocationProfile alp = lAD.next();
//            Thread thread = addThread(alp.getNode(), alp.getContext(), alp.getThread());
//            UserEvent userEvent = aeMap.get(new Integer(alp.getAtomicEventID()));
//            UserEventProfile userEventProfile = thread.getUserEventProfile(userEvent);
//
//            if (userEventProfile == null) {
//                userEventProfile = new UserEventProfile(userEvent);
//                thread.addUserEventProfile(userEventProfile);
//            }
//
//            userEventProfile.setNumSamples(alp.getSampleCount());
//            userEventProfile.setMaxValue(alp.getMaximumValue());
//            userEventProfile.setMinValue(alp.getMinimumValue());
//            userEventProfile.setMeanValue(alp.getMeanValue());
//            userEventProfile.setSumSquared(alp.getSumSquared());
//            userEventProfile.updateMax();
//        }

        downloadMetaData();

        databaseAPI.terminate();
        time = (System.currentTimeMillis()) - time;
        //System.out.println("Time to download file (in milliseconds): " + time);
        //System.out.println(time);

        // We actually discard the mean and total values by calling this
        // But, we need to compute other statistics anyway
        generateDerivedData();
    }
}
