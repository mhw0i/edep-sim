#include "EDepSimRootGeometryManager.hh"
#include "EDepSimTrajectoryPoint.hh"

#include <G4Track.hh>
#include <G4Step.hh>
#include <G4VProcess.hh>
#include <G4StepStatus.hh>
#include <G4ProcessType.hh>

#include <G4AttDefStore.hh>
#include <G4AttDef.hh>
#include <G4AttValue.hh>
#include <G4UnitsTable.hh>

#include <TGeoManager.h>

#include <EDepSimLog.hh>

G4String StepStatusToString(G4StepStatus status) {
    switch (status) {
        case G4StepStatus::fWorldBoundary:
            return "fWorldBoundary";
        // Step reached the world boundary
        case G4StepStatus::fGeomBoundary:
            return "fGeomBoundary";
        // Step defined by a geometry boundary
        case G4StepStatus::fAtRestDoItProc:
            return "fAtRestDoItProc";
        // Step defined by a PreStepDoItVector
        case G4StepStatus::fAlongStepDoItProc:
            return "fAlongStepDoItProc";
        // Step defined by a AlongStepDoItVector
        case G4StepStatus::fPostStepDoItProc:
            return "fPostStepDoItProc";
        // Step defined by a PostStepDoItVector
        case G4StepStatus::fUserDefinedLimit:
            return "fUserDefinedLimit";
        // Step defined by the user Step limit in the logical volume
        case G4StepStatus::fExclusivelyForcedProc:
            return "fExclusivelyForcedProc";
        // Step defined by an exclusively forced PostStepDoIt process
        case G4StepStatus::fUndefined:
            return "fUndefined";
        // Step not defined yet

        // Add other cases as necessary
        default:
            return "Unknown Status";
    }
}

G4Allocator<EDepSim::TrajectoryPoint> aTrajPointAllocator;

EDepSim::TrajectoryPoint::TrajectoryPoint()
    : fTime(0.), fMomentum(0.,0.,0.),
      fStepStatus(fUndefined),
      fProcessType(fNotDefined), fProcessSubType(0),
      fProcessName("NotDefined"), fPhysVolName("OutofWorld"),
      fPrevPosition(0,0,0) { }

EDepSim::TrajectoryPoint::TrajectoryPoint(const G4Step* aStep)
    : G4TrajectoryPoint(aStep->GetPostStepPoint()->GetPosition()) {
    fTime = aStep->GetPostStepPoint()->GetGlobalTime();
    fMomentum = aStep->GetPostStepPoint()->GetMomentum();
    fStepStatus = aStep->GetPostStepPoint()->GetStepStatus();
    if (aStep->GetPostStepPoint()->GetPhysicalVolume()) {
        fPhysVolName 
            = aStep->GetPostStepPoint()->GetPhysicalVolume()->GetName();
    }
    else {
        fPhysVolName == "OutOfWorld";
    }
    fPrevPosition = aStep->GetPreStepPoint()->GetPosition();
    // Check if the G4VProcess for the defining process is available.  It
    // isn't available for steps defined by the user, step limits, or some
    // other "bookkeeping" pseudo interactions.
    if (aStep->GetPostStepPoint()->GetProcessDefinedStep()) {
        fProcessType = aStep->GetPostStepPoint()->
            GetProcessDefinedStep()->GetProcessType();
        fProcessSubType = aStep->GetPostStepPoint()->
            GetProcessDefinedStep()->GetProcessSubType();
        fProcessName = aStep->GetPostStepPoint()->
            GetProcessDefinedStep()->GetProcessName();
        fProcessDeposit =  aStep->GetTotalEnergyDeposit();
    }
}

EDepSim::TrajectoryPoint::TrajectoryPoint(const G4Track* aTrack)
    : G4TrajectoryPoint(aTrack->GetPosition()) {
    fTime = aTrack->GetGlobalTime();
    fMomentum = aTrack->GetMomentum();
    fStepStatus = fUndefined;
    if (aTrack->GetVolume()) {
        fPhysVolName = aTrack->GetVolume()->GetName();
    }
    else {
        fPhysVolName == "OutOfWorld";
    }
    fPrevPosition = aTrack->GetPosition();
    const G4VProcess* proc = aTrack->GetCreatorProcess();
    if (proc) {
        fProcessType = proc->GetProcessType();
        fProcessSubType = proc->GetProcessSubType();
        fProcessName = proc->GetProcessName();
        fProcessDeposit =  0.0;
    }
}

EDepSim::TrajectoryPoint::TrajectoryPoint(const EDepSim::TrajectoryPoint &right)
    : G4TrajectoryPoint(right) {
    fTime = right.fTime;
    fMomentum = right.fMomentum;
    fStepStatus = right.fStepStatus;
    fProcessType = right.fProcessType;
    fProcessSubType = right.fProcessSubType;
    fProcessName = right.fProcessName;
    fProcessDeposit = right.fProcessDeposit;
    fPhysVolName = right.fPhysVolName;
    fPrevPosition = right.fPrevPosition;
}

EDepSim::TrajectoryPoint::~TrajectoryPoint() { }

const std::map<G4String,G4AttDef>* EDepSim::TrajectoryPoint::GetAttDefs() const {
    G4bool isNew;

    std::map<G4String,G4AttDef>* store
        = G4AttDefStore::GetInstance("EDepSim::TrajectoryPoint",isNew);

    if (isNew) {

        G4String Time("Time");
        (*store)[Time] =
            G4AttDef(Time, "Time", "Physics","G4BestUnit","G4double");

        G4String Momentum("Momentum");
        (*store)[Momentum] =
            G4AttDef(Momentum, "Momentum", "Physics",
                     "G4BestUnit","G4ThreeVector");

        G4String StepStatus("StepStatus");
        (*store)[StepStatus] =
            G4AttDef(StepStatus, "StepStatus", "Physics", "", "G4StepStatus");

        G4String VolumeName("VolumeName");
        (*store)[VolumeName] =
            G4AttDef(VolumeName, "VolumeName", "Physics", "", "G4String");

        G4String VolumeNode("VolumeNode");
        (*store)[VolumeNode] =
            G4AttDef(VolumeNode, "VolumeNode", "Physics", "", "G4int");

    }
    return store;
}

std::vector<G4AttValue>* EDepSim::TrajectoryPoint::CreateAttValues() const {
    std::vector<G4AttValue>* values = new std::vector<G4AttValue>;

    values->push_back(G4AttValue("Time",G4BestUnit(fTime,"Time"),""));

    values->push_back(G4AttValue("Momentum",
                                 G4BestUnit(fMomentum,"Momentum"),""));
    values->push_back(G4AttValue("StepStatus",StepStatusToString(fStepStatus),""));

    values->push_back(G4AttValue("PhysVolName",fPhysVolName,""));

#ifdef G4ATTDEBUG
    EDepSimInfo(G4AttCheck(values,GetAttDefs()));
#endif

    return values;
}

int EDepSim::TrajectoryPoint::GetVolumeNode() const {
    gGeoManager->PushPath();
    int node
        = EDepSim::RootGeometryManager::Get()->GetNodeId(0.5*(GetPosition()
                                                          +fPrevPosition));
    gGeoManager->PopPath();
    return node;
}
