/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef JSON_DESCRIPTOR_H_
#define JSON_DESCRIPTOR_H_

#include <boost/foreach.hpp>

#include "BaseDescriptor.h"
#include "../DataStructures/JSONDescriptionFactory.h"
#include "../Util/StringUtil.h"

template<class SearchEngineT>
class JSONDescriptor : public BaseDescriptor<SearchEngineT>{
private:
    _DescriptorConfig config;
    _RouteSummary summary;
    JSONDescriptionFactory descriptionFactory;
    std::string tmp;
    _Coordinate current;

public:
    JSONDescriptor() {}
    void SetConfig(const _DescriptorConfig & c) { config = c; }

    void Run(http::Reply & reply, RawRouteData &rawRoute, PhantomNodes &phantomNodes, SearchEngineT &sEngine, unsigned durationOfTrip) {
        WriteHeaderToOutput(reply.content);
        //We do not need to do much, if there is no route ;-)

        //INFO("Starting at " << sEngine.GetEscapedNameForNameID(phantomNodes.startPhantom.nodeBasedEdgeNameID) << ", id: " << phantomNodes.startPhantom.nodeBasedEdgeNameID);
        //INFO("Arriving at " << sEngine.GetEscapedNameForNameID(phantomNodes.targetPhantom.nodeBasedEdgeNameID) << ", id: " << phantomNodes.startPhantom.nodeBasedEdgeNameID);

        if(durationOfTrip != INT_MAX && rawRoute.routeSegments.size() > 0) {
            summary.startName = sEngine.GetEscapedNameForNameID(phantomNodes.startPhantom.nodeBasedEdgeNameID);
            summary.destName = sEngine.GetEscapedNameForNameID(phantomNodes.targetPhantom.nodeBasedEdgeNameID);
            summary.BuildDurationAndLengthStrings(0, durationOfTrip);
            reply.content += "0,"
                    "\"status_message\": \"Found route between points\",";
            descriptionFactory.AddToPolyline(phantomNodes.startPhantom.location);
            for(unsigned segmentIdx = 0; segmentIdx < rawRoute.routeSegments.size(); segmentIdx++) {
                const std::vector< _PathData > & path = rawRoute.routeSegments[segmentIdx];
                BOOST_FOREACH(_PathData pathData, path) {
                    sEngine.GetCoordinatesForNodeID(pathData.node, current);
                    descriptionFactory.AppendSegment(pathData, current);
                    if(pathData.turnInstruction != 0) {
                        INFO("Turn on " << sEngine.GetEscapedNameForNameID(pathData.nameID) << ", turnID: " << pathData.turnInstruction );
                    }
                }
            }
            descriptionFactory.AddToPolyline(phantomNodes.targetPhantom.location);
        } else {
            //no route found
            reply.content += "207,"
                    "\"status_message\": \"Cannot find route between points\",";
        }

        reply.content += "\"route_summary\": {"
                "\"total_distance\":";
        reply.content += summary.lengthString;
        reply.content += ","
                "\"total_time\":";
        reply.content += summary.durationString;
        reply.content += ","
                "\"start_point\":\"";
        reply.content += summary.startName;
        reply.content += "\","
                "\"end_point\":\"";
        reply.content += summary.destName;
        reply.content += "\"";
        reply.content += "},";
        reply.content += "\"route_geometry\": ";
        if(config.geometry) {
            if(config.encodeGeometry)
                descriptionFactory.AppendEncodedPolylineString(reply.content);
            else
                descriptionFactory.AppendUnencodedPolylineString(reply.content);
        } else {
            reply.content += "[]";
        }

        reply.content += ","
                "\"route_instructions\": [";
        if(config.instructions)
            descriptionFactory.AppendRouteInstructionString(reply.content);
        reply.content += "],";
        //list all viapoints so that the client may display it
        reply.content += "\"via_points\":[";
        for(unsigned segmentIdx = 1; (true == config.geometry) && (segmentIdx < rawRoute.segmentEndCoordinates.size()); segmentIdx++) {
            if(segmentIdx > 1)
                reply.content += ",";
            reply.content += "[";
            if(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location.isSet())
                convertInternalReversedCoordinateToString(rawRoute.segmentEndCoordinates[segmentIdx].startPhantom.location, tmp);
            else
                convertInternalReversedCoordinateToString(rawRoute.rawViaNodeCoordinates[segmentIdx], tmp);
            reply.content += tmp;
            reply.content += "]";
        }
        reply.content += "],"
                "\"transactionId\": \"OSRM Routing Engine JSON Descriptor (v0.2)\"";
        reply.content += "}";
    }

    void WriteHeaderToOutput(std::string & output) {
        output += "{"
                "\"version\": 0.3,"
                "\"status\":";
    }
};
#endif /* JSON_DESCRIPTOR_H_ */
